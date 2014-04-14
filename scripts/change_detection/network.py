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

import numpy
import networkx as nx
import senslab
from node import Node
import grapher

class Network(senslab.Network):
    def __init__(self, site, log_path, N, barN, alpha_0, sigma, output_split_logs = False):
        assert site is not None
        assert isinstance(site, str)
        assert log_path is not None
        assert isinstance(log_path, str)

        nodelogs = self._split_node_data(log_path, output_split_logs)

        self._initialized = False

        nodes = {}
        for nodeid, log in nodelogs.items():
            print " * Loading data for node nr. %d ..." % nodeid
            assert not (nodeid in nodes.keys())
        
            node = Node(site, nodeid, log, N, barN, alpha_0, sigma)

            if node.initialized():
                nodes[nodeid] = node
            else:
                print "warning: discarding data on node nr. %d since it failed to parse its log\n" % nodeid


        if len(nodes.keys()) == 0:
            print "No valid data loaded."
            return
    
        self._log_path = log_path
        self._epoch = 0
        self._M = nodes.values()[0].get_M()
        self._D = nodes.values()[0].get_D()
        self._N = N
        self._barN = barN
        self._alpha_0 = alpha_0
        self._sigma = sigma
        self._nodes = nodes

        self._initialized = True

        print "ChangeDetection network initialized with N=%d, barN=%d, alpha0=%.3f, sigma=%.3f" % (N, barN, alpha_0, sigma)
        self.print_info()

    def initialized(self):
        return self._initialized

    def get_epoch(self):
        assert self._initialized
        return self._epoch

    def step(self):
        assert self._initialized
	self._epoch += 1

        for node in self._nodes.values():
            node.tick_epoch()
            assert node.get_epoch() == self._epoch

        #
        # build the graph for this epoch
        #
        if 0:
            G = nx.Graph()
        else:
            G = nx.DiGraph()
        unknown_sync_ids = []
        unknown_data_ids = []
        for nodeid, node in self._nodes.items():
            G.add_node(nodeid)
            pos = node.get_position()
            
            #G.node[nodeid]['pos'] = [pos[i] for i in (0,2)]
            G.node[nodeid]['pos'] = [pos[i] for i in (0,1)]


            for senderid in node.get_data_packets(self._epoch):
                # Check that senderid is the node-id that we could load without errors
                if senderid in self._nodes.keys():
                    try:
                        G[nodeid][senderid]['data_weight'] += 1
                    except:
                        G.add_edge(nodeid, senderid, data_weight = 1.)
                else:
                    if senderid not in unknown_data_ids:
                        unknown_data_ids.append(senderid)

            for senderid in node.get_sync_packets(self._epoch):
                # Check that senderid is the node-id that we could load without errors
                if senderid in self._nodes.keys():
                    try:
                        G[nodeid][senderid]['sync_weight'] += 1
                    except:
                        G.add_edge(nodeid, senderid, sync_weight = 1.)
                else:
                    if senderid not in unknown_sync_ids:
                        unknown_sync_ids.append(senderid)

        if len(unknown_sync_ids):
            print "@%d unknown sync sources with board-id(s) %s" % (self._epoch, repr(unknown_sync_ids))

        if len(unknown_data_ids):
            print "@%d unknown data sources with board-id(s) %s" % (self._epoch, repr(unknown_data_ids))

        #print "G.edge", G.edge
        #print nx.get_edge_attributes(G, 'sync_weight')
        #print nx.get_edge_attributes(G, 'data_weight')
        return G


    def get_M(self):
        assert self._initialized
        return self._M


    def get_D(self):
        assert self._initialized
        return self._D


    def get_N(self):
        assert self._initialized
        return self._N


    def get_barN(self):
        assert self._initialized
        return self._barN


    def get_alpha0(self):
        assert self._initialized
        return self._alpha_0


    def get_sigma(self):
        assert self._initialized
        return self._sigma


    def last_epoch(self):
        assert self._initialized
        last_epoch = 0
        for node in self._nodes.values():
            last_epoch = max(last_epoch, node.last_epoch())

        return last_epoch

    def draw_graph(self):
        assert self._initialized
        grapher.draw_graph(self, self._log_path)
