/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <btc/ecc.h>
#include <btc/random.h>

#include "utest.h"
#include "utils.h"

void test_ecc()
{
    unsigned char r_buf[32];
    memset(r_buf, 0, 32);
    random_init();

    while (ecc_verify_privatekey(r_buf) == 0) {
        random_bytes(r_buf, 32, 0);
    }

    memset(r_buf, 0xFF, 32);
    u_assert_int_eq(ecc_verify_privatekey(r_buf), 0); //secp256k1 overflow

    uint8_t pub_key33[33], pub_key33_invalid[33], pub_key65[65], pub_key65_invalid[65];

    memcpy(pub_key33, utils_hex_to_uint8("02fcba7ecf41bc7e1be4ee122d9d22e3333671eb0a3a87b5cdf099d59874e1940f"), 33);
    memcpy(pub_key33_invalid, utils_hex_to_uint8("999999999941bc7e1be4ee122d9d22e3333671eb0a3a87b5cdf099d59874e1940f"), 33);
    memcpy(pub_key65, utils_hex_to_uint8("044054fd18aeb277aeedea01d3f3986ff4e5be18092a04339dcf4e524e2c0a09746c7083ed2097011b1223a17a644e81f59aa3de22dac119fd980b36a8ff29a244"), 65);
    memcpy(pub_key65_invalid, utils_hex_to_uint8("044054fd18aeb277aeedea01d3f3986ff4e5be18092a04339dcf4e524e2c0a09746c7083ed2097011b1223a17a644e81f59aa3de22dac119fd980b39999f29a244"), 65);


    u_assert_int_eq(ecc_verify_pubkey(pub_key33, 1), 1);
    u_assert_int_eq(ecc_verify_pubkey(pub_key65, 0), 1);

    u_assert_int_eq(ecc_verify_pubkey(pub_key33_invalid, 1), 0);
    u_assert_int_eq(ecc_verify_pubkey(pub_key65_invalid, 0), 0);
}
