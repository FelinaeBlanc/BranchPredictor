#include "predictor.h"
#include <math.h>

/////////////// STORAGE BUDGET JUSTIFICATION ////////////////

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

// Constructeur du prédicteur
PREDICTOR::PREDICTOR(char *prog, int argc, char *argv[])
{
   // La trace est tjs présente, et les arguments sont ceux que l'on désire
   if (argc != 2) {
      fprintf(stderr, "usage: %s <trace> pcbits countbits\n", prog);
      exit(-1);
   }

   uint32_t pcbits    = strtoul(argv[0], NULL, 0);
   uint32_t countbits = strtoul(argv[1], NULL, 0);

   nentries = (1 << pcbits);        // nombre d'entrées dans la table bimodale
   pcmask   = (nentries - 1);       // masque pour n'accéder qu'aux bits significatifs de PC
   countmax = (1 << countbits) - 1; // valeur max atteinte par le compteur à saturation
   table    = new uint32_t[nentries](); // table bimodale

   // if PRED 0 on change rien ...
   #if PRED == 1
      global_history = 0; // table global history
   #elif PRED == 2 // Pattern history table
      local_history = new uint32_t[nentries](); // table global history
   #elif PRED == 3 // 2 prédicteurs + métaprédicteur associé 
      global_history = 0; // table global history
      local_history = new uint32_t[nentries](); // table global history

      table_gshare = new uint32_t[nentries](); // table bimodale gshare
      table_pattern_local = new uint32_t[nentries](); // table bimodale pattern
      table_metapredicteur = new uint32_t[nentries](); // table meta predicteur
   #elif PRED == 4 // Perceptrons

        table_global_history = new int32_t[HISTORY_LENGTH];
        for (uint32_t i = 0; i < HISTORY_LENGTH; ++i) {
            table_global_history[i] = -1; // Initialiser à -1 (non pris)
        }

        // Initialiser la table des perceptrons
        table_perceptrons = new int32_t*[nentries];
        for (uint32_t i = 0; i < nentries; ++i) {
            table_perceptrons[i] = new int32_t[HISTORY_LENGTH + 1]; // +1 pour le biais
            for (uint32_t j = 0; j <= HISTORY_LENGTH; ++j) {
                table_perceptrons[i][j] = 0; 
            }
        }

   #endif
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

bool PREDICTOR::GetPrediction(UINT64 PC)
{
   #if PRED == 0
      uint32_t v = table[PC & pcmask];
      return v > countmax / 2 ? TAKEN : NOT_TAKEN;

   #elif PRED == 1 // Predicteur gshare
      // historique global
      uint32_t index = (global_history & pcmask) ^ (PC & pcmask);
      uint32_t v = table[index]; // Valeur table bimodale
      return v > countmax / 2 ? TAKEN : NOT_TAKEN;

   #elif PRED == 2 // Pattern history table
      uint32_t index_hist = (PC & pcmask);
      uint32_t index_bht = local_history[index_hist] & pcmask;
      uint32_t v = table[index_bht]; // Valeur table bimodale
      return v > countmax / 2 ? TAKEN : NOT_TAKEN;

   #elif PRED == 3  // 2 prédicteurs + métaprédicteur associé 

      // historique global Gshare
      uint32_t index = (global_history & pcmask) ^ (PC & pcmask);
      uint32_t v_gshare = table_gshare[index]; // Valeur table bimodale
      gshared_res = (v_gshare > countmax / 2 ? TAKEN : NOT_TAKEN);

      // pattern history
      uint32_t index_hist = (PC & pcmask);
      uint32_t index_bht = local_history[index_hist] & pcmask;
      uint32_t v_pattern = table_pattern_local[index_bht]; // Valeur table bimodale
      pattern_res = v_pattern > countmax / 2 ? TAKEN : NOT_TAKEN;

      uint32_t metapredicteur = table_metapredicteur[PC & pcmask];
      if (metapredicteur > 1){
         return gshared_res;
      } else {
         return pattern_res;
      }
      

   #elif PRED == 4 // Perceptrons
      // Les n entrées correspondent aux bits de l historique global de branchement
      uint32_t index = (PC & pcmask);
      int32_t* vec_perceptrons = table_perceptrons[index];

      // Calcul du produit scalaire
      y = vec_perceptrons[0]; // Poids du biais, x0=1
      for (uint32_t i = 0; i < HISTORY_LENGTH; ++i) {
         y += vec_perceptrons[i + 1] * table_global_history[i];
      }

      return y > 0 ? TAKEN : NOT_TAKEN;
   #endif
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void PREDICTOR::UpdatePredictor(UINT64 PC, OpType opType, bool resolveDir, bool predDir, UINT64 branchTarget)
{
   #if PRED == 0
      uint32_t v = table[PC & pcmask];
      table[PC & pcmask] = resolveDir == TAKEN ? SatIncrement(v, countmax) : SatDecrement(v);

   #elif PRED == 1 // Predicteur gshare
      uint32_t index = (global_history & pcmask) ^ (PC & pcmask);
      uint32_t v = table[index]; // valeur_t_bimodale
      table[index] = resolveDir == TAKEN ? SatIncrement(v, countmax) : SatDecrement(v);
      global_history = (global_history << 1) | (resolveDir == TAKEN);

   #elif PRED == 2
      uint32_t index_hist = (PC & pcmask);
      uint32_t index_bht = local_history[index_hist] & pcmask;

      uint32_t v = table[index_bht]; // valeur_t_bimodale
      table[index_bht] = resolveDir == TAKEN ? SatIncrement(v, countmax) : SatDecrement(v);
      local_history[index_hist] = (local_history[index_hist] << 1) | (resolveDir == TAKEN);

   #elif PRED == 3 
      uint32_t metapredicteur = table_metapredicteur[PC & pcmask];

      if (metapredicteur > 1){ // gshared_res
         if ((resolveDir == predDir) && (pattern_res != resolveDir)){
            table_metapredicteur[PC & pcmask] = SatIncrement(metapredicteur, 3);
         } else if ((resolveDir != predDir) && (pattern_res == resolveDir)){
            table_metapredicteur[PC & pcmask] = SatDecrement(metapredicteur);
         }
      } else { // pattern 
         if ((resolveDir == predDir) && (gshared_res != resolveDir)){
            table_metapredicteur[PC & pcmask] = SatDecrement(metapredicteur);
         } else if ((resolveDir != predDir) && (gshared_res == resolveDir)){
            table_metapredicteur[PC & pcmask] = SatIncrement(metapredicteur, 3);
         }
      }

      uint32_t index = (global_history & pcmask) ^ (PC & pcmask);
      uint32_t v_gshare = table_gshare[index]; // valeur_t_bimodale
      table_gshare[index] = resolveDir == TAKEN ? SatIncrement(v_gshare, countmax) : SatDecrement(v_gshare);
      global_history = (global_history << 1) | (resolveDir == TAKEN);

      uint32_t index_hist = (PC & pcmask);
      uint32_t index_bht = local_history[index_hist] & pcmask;

      uint32_t v_pattern = table_pattern_local[index_bht]; // valeur_t_bimodale
      table_pattern_local[index_bht] = resolveDir == TAKEN ? SatIncrement(v_pattern, countmax) : SatDecrement(v_pattern);
      local_history[index_hist] = (local_history[index_hist] << 1) | (resolveDir == TAKEN);

   #elif PRED == 4

      int32_t t = resolveDir == TAKEN ? 1 : -1;
      int32_t yout = 0;
      if (y > seuil){
         yout = 1;
      } else if(y < seuil){
         yout = -1;
      }

      if (yout != t){
         uint32_t index = (PC & pcmask);
         int32_t* vec_perceptrons = table_perceptrons[index];

         vec_perceptrons[0] = vec_perceptrons[0] + t; // Biais

         for (uint32_t i=0;i<HISTORY_LENGTH;i++){
            vec_perceptrons[i+1] = vec_perceptrons[i+1] + t*table_global_history[i];
         }
      }

      // Décale l'historique
      for (uint32_t i=HISTORY_LENGTH; i>0; i--) {
         table_global_history[i] = table_global_history[i-1];
      }
      table_global_history[0] = t;

   #endif

}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void PREDICTOR::TrackOtherInst(UINT64 PC, OpType opType, bool branchDir, UINT64 branchTarget)
{
   // This function is called for instructions which are not
   // conditional branches, just in case someone decides to design
   // a predictor that uses information from such instructions.
   // We expect most contestants to leave this function untouched.
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////


/***********************************************************/
