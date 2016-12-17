/* 
 * Copyright (C) 2016  Peter G. Jensen <root@petergjoel.dk>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
// Created by Peter G. Jensen on 12/9/16.
#define BOOST_TEST_MODULE PTrieSet
#include <boost/test/unit_test.hpp>

#include <ptrie.h>
#include "utils.h"

using namespace ptrie;

BOOST_AUTO_TEST_CASE(EmptyTest)
{
    set<> set;
    try_insert(set,
              [](size_t i){
                  binarywrapper_t data;
                  return data;
              },
            1);
}

BOOST_AUTO_TEST_CASE(InsertByte)
{
    set<> set;
    try_insert(set,
              [](size_t i){
                  binarywrapper_t data(8);
                  data.raw()[0] = (uchar)i;
                  return data;
              },
            256);
}

BOOST_AUTO_TEST_CASE(InsertByteSplit)
{
    set<128,sizeof(size_t)+1> set;
    try_insert(set,
              [](size_t i){
                  binarywrapper_t data(8);
                  data.raw()[0] = (uchar)i;
                  return data;
              },
            256);
}

BOOST_AUTO_TEST_CASE(HeapTest)
{
    set<sizeof(size_t)+1> set;
    try_insert(set,
              [](size_t i){
                  binarywrapper_t data;
                  data.copy((uchar*)&i, sizeof(size_t));
                  return data;
              },
            1024);
}

BOOST_AUTO_TEST_CASE(InsertMill)
{
    set<> set;
    try_insert(set,
              [](size_t i){
                  binarywrapper_t data;
                  data.copy((uchar*)&i, sizeof(size_t));
                  return data;
              },
            1024*1024);
}

BOOST_AUTO_TEST_CASE(PseudoRand1)
{
    for(size_t seed = 0; seed < 10; ++ seed) {
        set<> set;
        try_insert(set,
                  [seed](size_t i) {
                      return rand_data(seed + i, 256);
                  },
                  1024 * 10);
    }
}

BOOST_AUTO_TEST_CASE(PseudoRand2)
{
    for(size_t seed = 0; seed < 10; ++ seed) {
        set<> set;
        try_insert(set,
                  [seed](size_t i) {
                     return rand_data(seed + i, 16);
                  },
                  1024 * 10);
    }
}


BOOST_AUTO_TEST_CASE(PseudoRandSplitHeap)
{
    for(size_t seed = 42; seed < (42+10); ++seed) {
        set<sizeof(size_t)+1, 2> set;
        try_insert(set,
                  [seed](size_t i) {
                     return rand_data(seed + i, 16);
                  },
                  1024 * 10);
    }
}

BOOST_AUTO_TEST_CASE(InsertDeleteByte)
{
    set<> set;
    try_insert(set,
              [](size_t i){
                  binarywrapper_t data(8);
                  data.raw()[0] = (uchar)i;
                  return data;
              },
            256);
    for(int i = 255; i >= 0; --i)
    {
        binarywrapper_t data(8);
        data.raw()[0] = (uchar)i;
        bool res = set.erase(data);
        BOOST_REQUIRE_MESSAGE(res, "FAILED ON DELETE " << i << " BIN " << data << " ");

        auto exists = set.exists(data);
        BOOST_REQUIRE_MESSAGE(!exists.first, "FAILED ON DELETE, STILL EXISTS " << i << " BIN " << data << " ");
        data.release();
    }
}

