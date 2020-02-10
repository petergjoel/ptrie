/*
 * Copyright Peter G. Jensen <root@petergjoel.dk>
 *  
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * File:   binarywrapper.cpp
 * Author: Peter G. Jensen
 *
 * Created on 10 June 2015, 19:20
 */

#include <sstream>

#include "binarywrapper.h"
namespace ptrie
{
    
    size_t binarywrapper_t::overhead(uint size)
    {
        size = size % 8;
        if (size == 0)
            return 0;
        else
            return 8 - size; 
    }
    
    binarywrapper_t::binarywrapper_t(uint size)
    {
        _nbytes = (size + overhead(size)) / 8;
        _blob = zallocate(_nbytes);
    }
    
    binarywrapper_t::binarywrapper_t(uchar* raw, uint size)
    {
        _nbytes = size / 8 + (size % 8 ? 1 : 0);     
        _blob = raw;
        
        if(_nbytes <= __BW_BSIZE__)
            memcpy(const_raw(), raw, _nbytes);
        
//        assert(raw[0] == const_raw()[0]);
    }
    
}
