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


import os
import re
from node import Node

class Network():
    def __init__(self, site, log_path, output_split_logs = False):
        assert site is not None
        assert isinstance(site, str)
        assert log_path is not None
        assert isinstance(log_path, str)

        self._initialized = False
        self._nodes = None

        nodelogs = _split_node_data(log_path, output_split_logs)

        nodes = {}
        for nodeid, log in nodelogs.items():
            print " * Loading data for node nr. %d ..." % nodeid
            assert not (nodeid in nodes.keys())
        
            node = Node(site, nodeid, log)

            if node.initialized():
                nodes[nodeid] = node
            else:
                print "warning: discarding data on node nr. %d since it failed to parse its log\n" % nodeid

        self._nodes = nodes
        self._initialized = True


    def get_nodes(self):
        assert self._initialized
        return self._nodes


    def print_info(self):
        assert self._initialized
        #
        # show some info for the nodes that survived the faulty parsers :)
        #
        print "This experiment contains valid data from %d nodes:" % (len(self._nodes))
        for nodeid in sorted(self._nodes.keys()):
            node = self._nodes[nodeid]
            assert nodeid == node.get_id()
            print "  node nr.%3d/0x%.4x at position %s" % (nodeid, node.get_board_id16(), str(node.get_position()))


    @staticmethod
    def _split_node_data(log_path, output_split_logs):
        nodelogs = {}

        lines = file(log_path).readlines()
        for l in lines:
            m = re.search(r'^\[([0-9]+)\] (.*)', l)
            if m is not None:
                nodeid = int(m.group(1))
                text = m.group(2)
                if nodeid in nodelogs.keys():
                    nodelogs[nodeid].append(text)
                else:
                    nodelogs[nodeid] = [text]
            else:
                print " ! cannot match \"%s\"" % l
                continue

        if output_split_logs:
            experiment_dir = os.path.dirname(log_path)
            for nodeid in nodelogs.keys():
                nodelog_path = os.path.join(experiment_dir, ''.join(('node-', str(nodeid), '.log')))
                if not os.path.exists(nodelog_path):
                    print "Writing log for node with id %d" % nodeid
                    file(nodelog_path, 'w').writelines('\n'.join(nodelogs[nodeid]))

        return nodelogs


