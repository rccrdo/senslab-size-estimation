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
import time
import signal
import shutil
import numpy

from matplotlib import rc
rc('font', family='serif')
import matplotlib.pyplot as plt

import networkx as nx

import node

NODE_SIZE = 200


def draw_graph(network, experiment_path):
    assert os.path.isfile(experiment_path)
    experiment_dir = os.path.dirname(experiment_path)
    assert os.path.isdir(experiment_dir)
    assert network is not None

    experiment_filename = os.path.basename(experiment_path)
    
    M = network.get_M()
    D = network.get_D()
    N = network.get_N()
    barN = network.get_barN()
    alpha_0 = network.get_alpha0()
    sigma = network.get_sigma()

    # Create dirs in which frames and video will be saved
    simid_base = 'N%d_barN%d_alpha%.3f_sigma%.2f' % (N,barN,alpha_0,sigma)
    sim_dir = os.path.join(experiment_dir, simid_base)
    frames_dir = '%s/frames' % sim_dir
    video_path = '%s/%s.avi' % (sim_dir, simid_base)
    video_cmd = "avconv -y -r 2 -i %s/%%04d.jpg -b 2000k %s" % (frames_dir, video_path)

    if os.path.exists(sim_dir):
        shutil.rmtree(sim_dir)

    os.mkdir(sim_dir)
    os.mkdir(frames_dir)

    # Install SIGINT handler so that a video is produced even
    # if the user aborts the current simulation
    global simulation_stop
    simulation_stop = False

    def handler_sig_term(signum, frame):
        print '\nStopping simulation ...'
        global simulation_stop
        simulation_stop = True
        
    signal.signal(signal.SIGINT,handler_sig_term)


    agents = network.get_nodes()

    # Create the figure
    fig = plt.figure(figsize=(7.5,7.5))   
    plt.ion()
    plt.show()

    edge_color = (0.9,0.9,0.9)
    color_sleeping = (0.75,0.75,0.75)
    color_active = (1,1,1)
    color_starting = (1,0.9,0.)
    color_alarmed = (1,0.1,0.1)


    STOP_EPOCH = network.last_epoch()
    for h in xrange(1,STOP_EPOCH+1):
        if simulation_stop:
            # user sent us SIGINT, exit simulation loop
            break

        # step the network
        graph = network.step()
        t = network.get_epoch()
        print 'epoch %d/%d' % (t, STOP_EPOCH)

        # setup figure for this iteration
        fig.clf()
        fig.patch.set_facecolor((1,1,1))
        fig_border_width = 0.075
        ax = fig.add_axes([fig_border_width, fig_border_width, 1-2*fig_border_width, 1-2*fig_border_width])
        plt.axis('off')


        # Collect statistics to be displayed
        actives = []
        alarmed = []    
        alarms_count = 0
        alarmed_colors = []
        alarmed_data = {}
        starting = []
        sleeping = []

        min_first_k = D + 1
        max_first_k = 0
        min_last_k = D + 1
        max_last_k = 0
        for nodeid, agent in agents.items():
            state2 = agent.state2()
            if state2 == node.AGENT_STATE_STARTINGUP:
                starting.append(nodeid)
            elif state2 == node.AGENT_STATE_SS:
                info = agent.test_info()
                assert info is not None
          
                change_time, pre_change_size, loglambda, alarm = info
                # discard info on the first x-step neighborhoods:
                x=1
                for i in xrange(0,x):
                    alarm[i]=0

                sum_alarm = numpy.sum(alarm)
                if sum_alarm > 0:
                    alarms_count += sum_alarm
                    alarmed.append(nodeid)
                    first_k = alarm.index(1) 
                    alarm.reverse()
                    last_k = D - alarm.index(1)
                    alarm.reverse()
                    min_first_k = min(min_first_k, first_k+1)
                    max_first_k = max(max_first_k, first_k+1)
                    min_last_k = min(min_last_k, last_k)
                    max_last_k = max(max_last_k, last_k)

                    alarmed_data[nodeid] = first_k + 1
                    
                    alarmed_colors.append((1. - float(first_k)/float(D),0.1,0.1))
                else:
                    actives.append(nodeid)
          
            elif state2 == node.AGENT_STATE_SLEEPING:
                sleeping.append(nodeid)

        # Skip a few frames
        #if t< 18:
        #    continue

        pos = nx.get_node_attributes(graph,'pos')
        
        # Draw the network edges and nodes
        data_weights = nx.get_edge_attributes(graph,'data_weight')
        sync_weights = nx.get_edge_attributes(graph,'sync_weight')
        data_edges = data_weights.keys()
        sync_edges = sync_weights.keys()
        #print 'sync_edges', sync_edges, 'sync_weights', sync_weights
        #print 'data_edges', data_edges, 'data_weights', data_weights
        #width_data = []
        #for edge in edges:
        #    width_data.append(graph[edge[0]][edge[1]]['data_weight'])
        SYNC_EDGE_MULT = 0.75
        DATA_EDGE_MULT = 0.15
        data_weights = [x*DATA_EDGE_MULT for x in data_weights.values()]
        sync_weights = [x*SYNC_EDGE_MULT for x in sync_weights.values()]

        nx.draw_networkx_edges(graph, pos, data_edges, edge_color='r', width=data_weights, alpha=0.25, ax=ax)
        nx.draw_networkx_edges(graph, pos, sync_edges, edge_color='y', width=sync_weights, alpha=0.25, ax=ax)


        def _draw_nodes(ids, color):
            if len(ids) == len(color):
                for i in ids:
                    nx.draw_networkx_nodes(graph, pos, [i],  NODE_SIZE, node_color=color, ax=ax)    
            else:
                nx.draw_networkx_nodes(graph, pos, ids, NODE_SIZE, node_color=color, ax=ax)

        _draw_nodes(actives, color_active)
        _draw_nodes(starting, color_starting)
        _draw_nodes(sleeping, color_sleeping)
        #_draw_nodes(alarmed, alarmed_colors)

        if len(alarmed) == len(color_alarmed):
            for k in alarmed:
                nx.draw_networkx_nodes(graph, pos, [k],  NODE_SIZE, node_color=alarmed_colors, ax=ax)   
        else:
            nx.draw_networkx_nodes(graph, pos, alarmed,  NODE_SIZE, node_color=alarmed_colors, ax=ax)    


        # Text overlays
        plt.figtext(0.5, 0.925, "$M\,%d,\; \\alpha_0\, %.3f,\; D\, %d,\; N\, %d,\; \\overline{N}\, %d,\; \\sigma \, %.2f$" % (M, alpha_0, D, N, barN, sigma), family='serif', size=15, ha='center')

        stats_string = "$\\mathrm{epoch} \, %d,\; \mathrm{active}\, %d/%d,\;  \mathrm{alarms}\, %d/%d,\;$" % (t, len(actives) + len(alarmed) + len(starting), len(agents), len(alarmed), alarms_count)
        if len(alarmed):
            stats_string = ''.join([stats_string, "$\\lfloor k \\rfloor \, %d\leftrightarrow %d \; \\lceil k \\rceil \, %d\leftrightarrow %d$" % (min_first_k, max_first_k, min_last_k, max_last_k, )])
        else:
            stats_string = ''.join([stats_string, "$\\lfloor k \\rfloor \, \cdot\leftrightarrow \cdot \; \\lceil k \\rceil \, \cdot\leftrightarrow\cdot$"])
        plt.figtext(0.05, 0.05, stats_string, family='serif', size=15)

        # Axis-limits
        #plt.xlim((-float(grid_side)*0.1, float(grid_side-1)*1.1))
        #plt.ylim((-float(grid_side)*0.1, float(grid_side-1)*1.1))
        #plt.xlim(-0.1,2.6)
        #plt.ylim(-0.1,2.6)
        fig.canvas.draw()

        nodepos_str = "x y s1"
        for nodeid, agent in agents.items():

            xy = graph.node[nodeid]['pos']
            x = xy[0]
            y = xy[1]

            state = -1
            if nodeid in actives:
                state = 0

            if nodeid in alarmed:
                assert nodeid in alarmed_data

                state = alarmed_data[nodeid]
                assert state > 0

            if state > -1:
                nodepos_str +='\n'
                nodepos_str +='%.5f %.5f %d' % (x,y, state)


        file('%s/%d.NodesPositions' % (frames_dir,t), 'w').write(nodepos_str)
    
        linkslist_str = "xstart ystart towardsx towardsy"
        for e in data_edges:
            i = e[0]
            j = e[1]
            xy = graph.node[i]['pos']
            xi = xy[0]
            yi = xy[1]

            xy = graph.node[j]['pos']
            xj = xy[0]
            yj = xy[1]

            linkslist_str +='\n'
            linkslist_str +='%.5f %.5f %.5f %.5f' % (xi, yi, xj-xi, yj-yi)

        file('%s/%d.LinksList' % (frames_dir,t), 'w').write(linkslist_str)

        plt.savefig('%s/%.4d.jpg' % (frames_dir,t))
    
    # Call avconv to make a video from the saved frames
    os.system(video_cmd)
