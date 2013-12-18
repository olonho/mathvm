//
//  RegistryISAStaticHelpers.h
//  VM_3
//
//  Created by Hatless Fox on 12/17/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef VM_3_RegistryISAStaticHelpers_h
#define VM_3_RegistryISAStaticHelpers_h

#include <cassert>
#include <stdint.h>
#include <cstring>

//------------------------------------------------------------------------------
// Comparison helpers

//modes: 0 - l; 1 - le; 2 - eq; 3 - ne; 4 - ge; 5 - g
static int cmp_tbl[6][3] =
{ { 0, 1, 0}, //less
  { 0, 1, 1}, //less eq
  { 0, 0, 1}, //equal
  { 1, 1, 0}, //not equal
  { 1, 0, 1}, //greater eq
  { 1, 0, 0}  //greater
};

static uint64_t double_cmp(double a, double b, uint64_t mode) {
  int res = 0;
  if (a > b) { res = 0; }
  else if (a < b) { res = 1; }
  else { res = 2; }
  return cmp_tbl[mode][res];
}
static uint64_t int_cmp(int64_t a, int64_t b, int64_t mode) {
  int res = 0;
  if (a > b) { res = 0; }
  else if (a < b) { res = 1; }
  else { res = 2; }
  return cmp_tbl[mode][res];
}

//------------------------------------------------------------------------------
// Global Variable helpers

static uint64_t loadGlobalVar(uint64_t gvt, uint64_t fn_id, uint64_t var_id) {
  uint64_t *functionsGVT = (uint64_t *)gvt;
  assert((uint64_t *)functionsGVT[fn_id]);
  return ((uint64_t *)functionsGVT[fn_id])[var_id];
}
static void storeGlobalVar(uint64_t gvt, uint64_t fn_id,
                           uint64_t var_id, uint64_t value) {
  uint64_t *functionsGVT = (uint64_t *)gvt;
  assert((uint64_t *)functionsGVT[fn_id]);
  ((uint64_t *)functionsGVT[fn_id])[var_id] = value;
}

static void pushGVT(uint64_t gvt, uint64_t fn_id, uint64_t var_cnt) {
  uint64_t *functionsGVT = (uint64_t *)gvt;
  
  uint64_t *table = (uint64_t *)calloc(var_cnt + 1, sizeof(uint64_t));
  *table = functionsGVT[fn_id];
  functionsGVT[fn_id] = (uint64_t)table;
}

static void popGVT(uint64_t gvt, uint64_t fn_id) {
  uint64_t *functionsGVT = (uint64_t *)gvt;
  
  uint64_t *table = (uint64_t *)functionsGVT[fn_id];
  functionsGVT[fn_id] = *table;
  free(table);
}

#endif