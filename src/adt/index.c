/**
 * @implements index.h
 */

/* set log level for prints in this file */
#include <time.h>
#define LOG_LEVEL LOG_LEVEL_DEBUG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h> // for LINE_MAX
#include <math.h>
#include <stdbool.h>

#include "../../include/printing.h"
#include "../../include/adt/index.h"
#include "../../include/defs.h"
#include "../../include/common.h"
#include "../../include/adt/list.h"
#include "../../include/adt/map.h"
#include "../../include/adt/set.h"








//Operasjon nodetyper for parsing av syntakstre
typedef struct treenode{
    //Måte å håndtere ordene mine
    //Bruke dette for operatorene også
    int type;
    //Bruke bool tester for å se om det er operatorer eller ikke
    struct treenode *right;
    struct treenode *left;

    char *word;


}node_t;



typedef struct index{
    //Mapping av ordene som er i dokumentene
    map_t *map;
    //Mapping av dokumentene og ordene de inneholder
    size_t doc_counter;
    //Holder styr på antall ord
    size_t word_counter;
}index_t;




/**
 * You may utilize this for lists of query results, or write your own comparison function.
 */
ATTR_MAYBE_UNUSED
int compare_results_by_score(query_result_t *a, query_result_t *b) {
    if (a->score > b->score) {
        return -1;
    }
    if (a->score < b->score) {
        return 1;
    }
    return 0;
}

/**
 * @brief debug / helper to print a list of strings with a description.
 * Can safely be removed, but could be useful for debugging/development.
 *
 * Remove this function from your finished program once you are done
 */
ATTR_MAYBE_UNUSED
static void print_list_of_strings(const char *descr, list_t *tokens) {
    if (LOG_LEVEL <= LOG_LEVEL_INFO) {
        return;
    }
    list_iter_t *tokens_iter = list_createiter(tokens);
    if (!tokens_iter) {
        /* this is not a critical function, so just print an error and return. */
        pr_error("Failed to create iterator\n");
        return;
    }

    pr_info("\n%s:", descr);
    while (list_hasnext(tokens_iter)) {
        char *token = (char *) list_next(tokens_iter);
        pr_info("\"%s\", ", token);
    }
    pr_info("\n");

    list_destroyiter(tokens_iter);
}

index_t *index_create() {
    index_t *index = malloc(sizeof(index_t));
    if (index == NULL) {
        pr_error("Failed to allocate memory for index\n");
        return NULL;
    }



   index -> map = map_create((cmp_fn)strcmp, hash_string_fnv1a64);
   if(index-> map == NULL){
    printf("Map for words in doc failed");
    return NULL;
   }



   index -> doc_counter = 0;
   index -> word_counter = 0;


    return index;
}

void index_destroy(index_t *index) {
    // during development, you can use the following macro to silence "unused variable" errors.
    UNUSED(index);

    //iterasjon?
    /**
     * TODO: Free all memory associated with the index
     */

     //Ødelegger mappen over ordene i dokumentet
     if(index -> map != NULL){
        map_destroy(index->map, free, NULL);
     }


     free(index);
     
}

int index_document(index_t *index, char *doc_name, list_t *terms) {
    /**
     * TODO: Process document, enabling the terms and subsequent document to be found by index_query
     *
     * Note: doc_name and the list of terms is now owned by the index. See the docstring.
     */

     /*Blir splittet der vi sjekker om ordet finnes deretter dersom ordet allerede eksisterer*/

    //lager iterator for å gå gjennom ordene i dokumentet
    list_iter_t *iter = list_createiter(terms);

     while(list_hasnext(iter)){
        char *item = list_next(iter);

        //Sjekker om ordet finnes fra før av
        entry_t *word = map_get(index -> map, item);
        //printf("Item:%s ", item);

        if(word == NULL){
            //Dersom ordet ikke finnes fra før av, så lages det en ny liste for dokumentene
            list_t *doc_list = list_create((cmp_fn)strcmp);
            if(doc_list == NULL){
                //Ødelegger iteratoren dersom det ikke går å opprette
                list_destroyiter(iter);
                return -1;
                
            }
            //Øker antallet for de unike ordene i indexen
            index -> word_counter++;
            //Legger til dokumentet i den nye listen for ordet
            list_addlast(doc_list, strdup(doc_name));
            //Lagrer ordet i indexen
            map_insert(index -> map, strdup(item), doc_list);
        
        
        }else {
            //Om ordet allerede eksisterer, så hentes listen og sjekker om doc finnes
            list_t *doc_list = word -> val;
            int found_doc = 0;

            //Sjekker om den allerede eksistere i listen (Dokumentet) for ordet
            list_iter_t *doc_iter = list_createiter(doc_list);
            while(list_hasnext(doc_iter)){
                char *existing_doc = list_next(doc_iter);
                //printf("Checking: %s == %s\n", existing_doc, doc_name);

                if(strcmp(existing_doc, doc_name) == 0){
                    //printf("Found document: %s\n", doc_name);
                    //Ordet finnes allerede i listen
                    found_doc = 1;
                } 
                

            }
            list_destroyiter(doc_iter);

            //Om ordet ikke finnes så legges den til
            if(found_doc == 0){
                list_addlast(doc_list, strdup(doc_name));
                //list_destroyiter(iter);
            }

        }

     }
    //Frigjør iterator
    list_destroyiter(iter);
    //Øker antall dokumenter i index
    index -> doc_counter++;

    return 0; // or -x on error
}

/*  AST & PARSING */

/*Implementasjon av en query parser som representerer AST
 - For å håndtere BNF grammar så brukes det en rekursive descent parser
 - Parse de forskjellige gitte operatorene
*/
static node_t *Create_Node(list_t *query_tokens){
    //Allokere minne for den nye noden
    node_t *new_node = malloc(sizeof(node_t));
    if(new_node == NULL){
        return NULL;
    }
    
    char *token = list_popfirst(query_tokens);

    list_iter_t *token_iter = list_createiter(query_tokens);
    if(!list_hasnext(token_iter)){
        list_destroyiter(token_iter);
        new_node -> type = 99; // Telles som en term
        new_node -> word = strdup(token);
        new_node -> left = 0;
        new_node -> right = 0;

        return new_node;
        
    }

    if(strcmp(token, "&&")== 0 ){
        new_node -> type = 0;
        new_node -> left = Create_Node(query_tokens);
        new_node -> right = Create_Node(query_tokens);
        printf("Det går ja: %s", token);

        if(new_node -> left == NULL || new_node -> right == NULL){
            if(new_node -> left){
                free(new_node -> left);
                free(new_node -> right);
            }
        }
    }else if (strcmp(token,"||") == 0) {
        new_node -> type = 1;
        new_node -> left = Create_Node(query_tokens);
        new_node -> right = Create_Node(query_tokens);
    
    }else if (strcmp(token, "!") == 0) {
        new_node -> type = 2;
        new_node -> left = Create_Node(query_tokens);
        new_node -> right = Create_Node(query_tokens);
    }

    
    
   


    return new_node;
}


list_t* index_query(index_t* index, list_t* query_tokens, char* errmsg) {
    print_list_of_strings("query", query_tokens); // For debugging
    
 
    
    return NULL;
}

void index_stat(index_t *index, size_t *n_docs, size_t *n_terms) {
    /**
     * TODO: fix this
     */
    *n_docs = index->doc_counter;
    *n_terms = index->word_counter;
}


/*
1. f. Bygge AST -> Bygge opp treet som går gjennom treet for å sjekke verdien
2. Eval av treet -> eg skal bruke rbtree: for eval av operatorer
 - 1. Hente liste -> inn på et set -> utføre setoperasjonene(Union, diff og inter) -> Nytt set som skal returneres som en liste
3.
4. Kalkulere score -> TF-IDF





*/