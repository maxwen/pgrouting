#include <sqlite3.h>
#include "trsp.h"
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG 1

#ifdef DEBUG
#define DBG(format, arg...)                     \
    fprintf(stderr, format , ## arg)
#else
#define DBG(format, arg...) do { ; } while (0)
#endif

static sqlite3 *db;

static void
finish()
{
  DBG("In finish");
  sqlite3_close(db);
}

static int compute_trsp_cpp(
    char* sql,
    int dovertex,
    int start_id,
    double start_pos,
    int end_id,
    double end_pos,
    bool directed, 
    bool has_reverse_cost,
    char* restrict_sql,
    path_element_t **path,
    int *path_count) 
{
  edge_t *edges = NULL;
  int total_tuples = 0;

  restrict_t *restricts = NULL;
  int total_restrict_tuples = 0;

  int v_max_id=0;
  int v_min_id=INT_MAX;

  /* track if start and end are both in edge tuples */
  int s_count = 0;
  int t_count = 0;
  sqlite3_stmt* compiledStatement;

  char *err_msg;
  int ret = -1;
  register int z;

  DBG("start turn_restrict_shortest_path\n");
  
  ret = sqlite3_prepare(db, sql, -1, &compiledStatement, 0);
  if (ret != SQLITE_OK) {
       finish();
       return -1;
  }

  total_tuples = 0;
  while (sqlite3_step(compiledStatement) == SQLITE_ROW) {
        total_tuples++;
  }
  DBG("num edges %d\n", total_tuples);

  edges = malloc(total_tuples * sizeof(edge_t));
  sqlite3_reset(compiledStatement);
  int edgeCount = 0;
  while (sqlite3_step(compiledStatement) == SQLITE_ROW) {
        edge_t* e = &edges[edgeCount];
        e->id = sqlite3_column_int(compiledStatement, 0);
        e->source = sqlite3_column_int(compiledStatement, 1);
        e->target = sqlite3_column_int(compiledStatement, 2);
        e->cost = sqlite3_column_double(compiledStatement, 3);
        e->reverse_cost = sqlite3_column_double(compiledStatement, 4);
        edgeCount++;
        
        if(e->source<v_min_id)
            v_min_id=e->source;
  
        if(e->source>v_max_id)
            v_max_id=e->source;

        if(e->target<v_min_id)
            v_min_id=e->target;

        if(e->target>v_max_id)
            v_max_id=e->target;  
  }

  sqlite3_finalize(compiledStatement);
  
  ret = sqlite3_prepare(db, restrict_sql, -1, &compiledStatement, 0);
  if (ret != SQLITE_OK) {
       finish();
       return -1;
  }

  total_restrict_tuples = 0;
  while (sqlite3_step(compiledStatement) == SQLITE_ROW) {
        total_restrict_tuples++;
  }
  DBG("num restrictions %d\n", total_restrict_tuples);
  restricts = malloc(total_restrict_tuples * sizeof(restrict_t));
  sqlite3_reset(compiledStatement);
  int restrictCount = 0;
  while (sqlite3_step(compiledStatement) == SQLITE_ROW) {
        restrict_t* r = &restricts[restrictCount];
        
        for(int i=0; i<MAX_RULE_LENGTH;++i)
            r->via[i] = -1;
    
        r->target_id = sqlite3_column_int(compiledStatement, 0);
        r->to_cost = sqlite3_column_double(compiledStatement, 1);
        const char* str = sqlite3_column_text(compiledStatement, 2);
        if (str != NULL) {
            char* pch = NULL;
            int ci = 0;

            pch = (char *)strtok ((char *)str," ,");

            while (pch != NULL && ci < MAX_RULE_LENGTH)
            {
                r->via[ci] = atoi(pch);
                //DBG("    rest->via[%i]=%i\n", ci, r->via[ci]);
                ci++;
                pch = (char *)strtok (NULL, " ,");
            }
        }
        restrictCount++;
  }

  sqlite3_finalize(compiledStatement);
	
  //::::::::::::::::::::::::::::::::::::  
  //:: reducing vertex id (renumbering)
  //::::::::::::::::::::::::::::::::::::
  for(z=0; z<total_tuples; z++) {
    //check if edges[] contains source and target
    if (dovertex) {
        if(edges[z].source == start_id || edges[z].target == start_id)
          ++s_count;
        if(edges[z].source == end_id || edges[z].target == end_id)
          ++t_count;
    }
    else {
        if(edges[z].id == start_id)
          ++s_count;
        if(edges[z].id == end_id)
          ++t_count;
    }

    edges[z].source-=v_min_id;
    edges[z].target-=v_min_id;
    edges[z].cost = edges[z].cost;
    //DBG("edgeID: %i SRc:%i - %i, cost: %f\n", edges[z].id,edges[z].source, edges[z].target,edges[z].cost);      
    
  }
  DBG("Min vertex id: %i , Max vid: %i\n",v_min_id,v_max_id);

  if(s_count == 0) {
    fprintf(stderr, "Start id was not found.");
    finish();
    return -1;
  }
      
  if(t_count == 0) {
    fprintf(stderr, "Target id was not found.");
    finish();
    return -1;
  }
  
  if (dovertex) {
      start_id -= v_min_id;
      end_id   -= v_min_id;
  }

  if (dovertex) {
      DBG("Calling trsp_node_wrapper\n");
      ret = trsp_node_wrapper(edges, total_tuples, 
                        restricts, total_restrict_tuples,
                        start_id, end_id,
                        directed, has_reverse_cost,
                        path, path_count, &err_msg);
  }
  else {
      DBG("Calling trsp_edge_wrapper\n");
      ret = trsp_edge_wrapper(edges, total_tuples, 
                        restricts, total_restrict_tuples,
                        start_id, start_pos, end_id, end_pos,
                        directed, has_reverse_cost,
                        path, path_count, &err_msg);
  }

  DBG("*path_count = %i\n", *path_count);

  //::::::::::::::::::::::::::::::::
  //:: restoring original vertex id
  //::::::::::::::::::::::::::::::::
  for(z=0;z<*path_count;z++) {
    //DBG("vertex_id %i edge_id %i\n",(*path)[z].vertex_id, (*path)[z].edge_id);
    if (z || (*path)[z].vertex_id != -1)
        (*path)[z].vertex_id+=v_min_id;
  }

  DBG("ret = %i\n", ret);


  free(edges);
  free(restricts);
  finish();
  return ret;
}

int compute_trsp(
    char* db_file,
    char* sql,
    char* restrict_sql,
    int dovertex,
    int start_id,
    double start_pos,
    int end_id,
    double end_pos,
    path_element_t **path,
    int *path_count) 
{
    int rc = sqlite3_open(db_file, &db);

    return compute_trsp_cpp(sql, dovertex, start_id, start_pos, end_id, end_pos, false, true, restrict_sql, path, path_count);
}
