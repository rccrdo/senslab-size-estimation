# Copyright (c) 2012 Riccardo Lucchese, lucchese at dei.unipd.it
#               2012 Damiano Varagnolo, varagnolo at dei.unipd.it
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
#    1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
#
#    2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
#
#    3. This notice may not be removed or altered from any source
#    distribution.


import re
import siteinfo

class Node:
    def __init__(self, site, nodeid, nodelog):
        assert nodelog is not None
        assert len(nodelog) > 0
        assert nodeid == int(nodeid)
        assert nodeid >= 1 and nodeid<=256

        self._initialized = False

        self._node_id = nodeid
        self._board_id64 = None
        self._board_id16 = None
        self._xfer_crc = None

        if not siteinfo.exists(site):
            print "error: unknown site %s" % site
            return

        i = 0
        while i < len(nodelog):
            line = nodelog[i]
            if line.startswith("Rime started with address"):
                nodelog.pop(i)
                continue

            if line.startswith("MAC") and "Contiki 2.5 started" in line:
                nodelog.pop(i)
                continue

            if line.startswith("board-id64"):
                if self._board_id64 is None:
                    m = re.search(r'^board-id64 (0x[0-9a-f]+)$', line)
                    if m is not None:
                        self._board_id64 = int(m.group(1),16)
                        self._board_id16 = (self._board_id64 & 0x0000ff0000000000) >> 32 | (self._board_id64 & 0x00ff000000000000) >> 48

                        #print "node-id64/16 0x%.16x/0x%.4x" % (self._board_id64, self._board_id16)
                nodelog.pop(i)
                continue

            if line.startswith("xfer crc"):
                m = re.search(r'^xfer crc([0-9]+)$', line)
                if m is not None:
                    self._xfer_crc = int(m.group(1))

                    #print "node-id64/16 0x%.16x/0x%.4x" % (self._board_id64, self._board_id16)
                    nodelog.pop(i)
                    continue

            # go to the next line
            i += 1

        if self._board_id64 is None:
            print "warning: board-id entry not found"
            return
            
        if self._board_id16 != siteinfo.get_boardid(site, nodeid):
            print "warning: board-id16 mismatch"
            return

        self._pos = siteinfo.get_position(site, nodeid)
        if self._pos is None:
            print "warning: cannot retrieve the position for this node"
            return
        
        self._initialized = True

    def initialized(self):
        return self._initialized

    def get_id(self):
        assert self._initialized
        return self._node_id

    def get_board_id16(self):
        assert self._initialized
        return self._board_id16

    def get_board_id64(self):
        assert self._initialized
        return self._board_id64

    def get_position(self):
        assert self._initialized
        return self._pos
