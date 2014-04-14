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


import sys
import re
import numpy
import senslab
from itertools import chain

AGENT_STATE_STARTINGUP = 0
AGENT_STATE_SS = 1
AGENT_STATE_SLEEPING = 2

class Node(senslab.Node):
    _log_lambdas_dict = {
      # M, alpha_0
      
      (100, 0.001) : (-4.6258, -5.7310, -6.5785, -7.4075, -7.8890, -8.6315, -9.1293, -9.7421, -10.2092, -10.8044, -11.1716, -11.7202, -12.1787, -12.7187, -13.1546, -13.5228, -14.0023, -14.4407, -15.0668, -15.2590, -15.6473, -16.0777, -16.6401, -17.0700, -17.2838, -17.8285, -18.0989, -18.5672, -19.0675, -19.4022, -19.7850, -20.0924, -20.4988, -20.8993, -21.4015, -21.6770, -22.0752, -22.4945, -22.8047, -23.1486, -23.5578, -24.0108, -24.3404, -24.6122, -24.8441),
      (100, 0.002) : (-4.0361, -5.0768, -5.8435, -6.5506, -7.1939, -7.8121, -8.3082, -8.8130, -9.4610, -9.8656, -10.3205, -10.8089, -11.2051, -11.8459, -12.2233, -12.6325, -13.0812, -13.4472, -13.8587, -14.2789, -14.8628, -15.0815, -15.5238, -15.8732, -16.2977, -16.7576, -17.0820, -17.4868, -17.8004, -18.1797, -18.6755, -19.0763, -19.5020, -19.7495, -20.1664, -20.5165, -20.9163, -21.1314, -21.6004, -22.0161, -22.2849, -22.6914, -22.9736, -23.4465, -23.7008),      
      (100, 0.01) : (-2.6281, -3.5352, -4.2691, -4.8135, -5.4607, -5.9277, -6.4615, -6.9446, -7.3462, -7.8565, -8.3408, -8.7262, -9.1157, -9.4856, -9.9376, -10.3351, -10.7137, -11.0715, -11.5118, -11.8877, -12.2451, -12.6005, -13.0207, -13.4071, -13.7110, -14.1408, -14.4646, -14.8051, -15.2533, -15.4879, -15.9610, -16.3159, -16.6717, -16.9199, -17.2950, -17.6596, -17.9332, -18.3563, -18.5853, -18.9606, -19.3234, -19.7389, -20.0948, -20.2710, -20.7182),
      (100, 0.05) : (-1.3017, -2.0300, -2.6257, -3.1430, -3.6166, -4.0265, -4.4747, -4.8707, -5.2945, -5.6645, -6.0665, -6.4173, -6.7932, -7.1844, -7.5122, -7.8681, -8.2139, -8.5729, -8.9107, -9.2642, -9.5958, -9.9377, -10.2647, -10.6106, -10.9299, -11.2385, -11.5807, -11.8931, -12.1992, -12.5368, -12.8596, -13.1546, -13.4979, -13.7966, -14.1156, -14.4016, -14.7516, -15.0426, -15.3615, -15.6613, -15.9601, -16.3065, -16.6045, -16.9138, -17.2020),
      (50,0.01) : (-2.5771, -3.4889, -4.1979, -4.8285, -5.3300, -5.8381, -6.3518, -6.8376, -7.2610, -7.7090, -8.1500, -8.5712, -8.9667, -9.3936, -9.8237, -10.2738, -10.5618, -10.9052, -11.3376, -11.6982, -12.0512, -12.4544, -12.8300, -13.1436, -13.5439, -13.9202, -14.2774, -14.5703, -14.9666, -15.3226, -15.6311, -16.0204, -16.3657, -16.6253, -16.9531, -17.3763, -17.6466, -18.0764, -18.3800, -18.7494, -18.9519, -19.3746, -19.6124, -20.0182, -20.4390),
      (50, 0.05) : (-1.2704, -2.0016, -2.5753, -3.0916, -3.5540, -3.9869, -4.4003, -4.8128, -5.2163, -5.6160, -5.9588, -6.3524, -6.7083, -7.0194, -7.3843, -7.7206, -8.1068, -8.4173, -8.7564, -9.0936, -9.4432, -9.7914, -10.0662, -10.4666, -10.7379, -11.0513, -11.3435, -11.6904, -12.0084, -12.3080, -12.6399, -12.9290, -13.2691, -13.5290, -13.8847, -14.1689, -14.4879, -14.7850, -15.1033, -15.3961, -15.6525, -15.9912, -16.2659, -16.5944, -16.8643)
      }

    def __init__(self, site, nodeid, nodelog, N, barN, alpha_0, sigma):
        assert isinstance(N, int) and N > 0
        assert isinstance(barN, int) and barN >= 1 and barN < N
        assert alpha_0 > 0. and alpha_0 < 1.
        assert sigma > 0. and sigma <= 1.

        # init our parent
        senslab.Node.__init__(self, site, nodeid, nodelog)

        if not self._initialized:
            #
            # senslab.Node migth have found corrupt data, bail
            #
            return


        self._initialized = False

        if not senslab.siteinfo.exists(site):
            print "warning: unknown site %s" % site
            return

        sync_packets_at_epoch = {}
        data_packets_at_epoch = {}
        logstats_at_epoch = {}
        
        M = None
        D = None

        i = 0
        while i < len(nodelog):
            line = nodelog[i]
            #print line

            #
            # track sync packets
            #
            m = re.search(r'^@([0-9]+) track sync(.*)$', line)
            if m is not None:
                epoch = int(m.group(1))
                ids = m.group(2).strip()
                if not len(ids):
                    nodelog.pop(i)
                    continue

                matched = True
                ids = ids.split(' ')
                for _id in ids:
                    board_id16 = None
                    count = None
                    mm = re.search(r'^([0-9a-f]+):([0-9]+)', _id)
                    if mm is None:
                        matched = False
                        break

                    board_id16 = int(mm.group(1), 16)
                    count = int(mm.group(2)) 

                    senderid = senslab.siteinfo.get_nodeid_from_boardid(site, board_id16)
                    if epoch not in sync_packets_at_epoch.keys():
                        sync_packets_at_epoch[epoch] = [senderid]*count
                    else:
                        sync_packets_at_epoch[epoch].extend([senderid]*count)

                if matched:
                    nodelog.pop(i)
                    continue

            #
            # track data packets
            #
            m = re.search(r'^@([0-9]+) track data(.*)$', line)
            if m is not None:
                epoch = int(m.group(1))
                ids = m.group(2).strip()
                if not len(ids):
                    nodelog.pop(i)
                    continue


                matched = True
                ids = ids.split(' ')
                for _id in ids:
                    mm = re.search(r'^([0-9a-f]+):([0-9]+)', _id)
                    if mm is None:
                        matched = False
                        break
                    board_id16 = int(mm.group(1), 16)
                    count = int(mm.group(2)) 

                    senderid = senslab.siteinfo.get_nodeid_from_boardid(site, board_id16)
                    if epoch not in data_packets_at_epoch.keys():
                        data_packets_at_epoch[epoch] = [senderid]*count
                    else:
                        data_packets_at_epoch[epoch].extend([senderid]*count)

                if matched:
                    nodelog.pop(i)
                    continue


            #
            # find the sufficient statistics
            #
            m = re.search(r'^@([0-9]+) stats (.*)', line)
            if m is not None:
                if M is None or M <=0:
                    #
                    # we should have known M at this point
                    # 
                    break
                epoch = int(m.group(1))
                stats_str = m.group(2).split(' ')
                logstats = []
                estim = []
                for x in stats_str:
                    mm = re.search(r'^([0-9a-f]+).([0-9\-]+)', x)
                    
                    val = float(int(mm.group(1), 16))/float(0x100000000)
                    exp = int(mm.group(2)) 
                
                    logstat = -(numpy.log(val) + exp*numpy.log(2))
                    logstats.append(logstat)
                    M = 100
                    hatS = 1.
                    if logstat != 0:
                        hatS = float(M)/logstat

                    estim.append(hatS)
            
                assert epoch not in logstats_at_epoch.keys()
                logstats_at_epoch[epoch] = logstats
                nodelog.pop(i)
                continue


            m = re.search(r'^@([0-9]+) size-estimator recv packet ids (.*)$', line)
            if m is not None:
                nodelog.pop(i)
                continue

            m = re.search(r'^@([0-9]+) data xfer crc mismatch$', line)
            if m is not None:
                nodelog.pop(i)
                continue

            m = re.search(r'^@([0-9]+) sync xfer crc mismatch$', line)
            if m is not None:
                nodelog.pop(i)
                continue

            if line.startswith("size-estimator: data sent in "):
                nodelog.pop(i)
                continue

            if line.startswith("size-estimator: packet ids "):
                nodelog.pop(i)
                continue

            if line.startswith("epoch-syncer: sync offsets "):
                nodelog.pop(i)
                continue

            if line.startswith("size-estimator: waiting for epoch end"):
                nodelog.pop(i)
                continue

            if line.startswith("epoch-syncer: epoch ") and line.endswith(' ended'):
                nodelog.pop(i)
                continue

            if line.startswith("Starting 'epoch-syncer' 'size-estimator'"):
                nodelog.pop(i)
                continue

            if line.startswith("epoch-syncer: adjusting "):
                nodelog.pop(i)
                continue

            if line.startswith("size-estimator: jumping epoch "):
                nodelog.pop(i)
                continue

            if line.startswith("size-estimator: M="):
                m = re.search(r'size-estimator: M=([0-9]+), D=([0-9]+)', line)
                if m is not None:
                    M = int(m.group(1))
                    D = int(m.group(2))

                    nodelog.pop(i)
                    continue

            if line.startswith("epoch interval "):
                nodelog.pop(i)
                continue

            if line.startswith("size-estimator: discard packet from epoch"):
                print "warning: size-estimator lost epoch synchronism"
                return

            # go to the next line
            print line
            i += 1

        if (M is None) or (M<=0):
            print "warning: M has non valid value %s" % (repr(M))
            return

        if (D is None) or (D<=0):
            print "warning: N has non valid value %s" % (repr(D))
            return

        consensus_epochs = sorted(logstats_at_epoch.keys())
        nr_consensus_epochs = len(consensus_epochs)
        if not nr_consensus_epochs:
            print "warning: no size-estimation statistics found"

        if not (M, alpha_0) in self._log_lambdas_dict.keys():
            print "warning: missing test thresholds for M=%d and alpha_0=%.f" % (M, alpha_0)
            return
            
        #
        # we are done parsing the log, init the change-detector
        #
        self._log_lambdas = self._log_lambdas_dict[(M,alpha_0)]
        assert len(self._log_lambdas) > N

        self._M = M
        self._M__1_minus_logM__ = M*(1. -numpy.log(M))
        self._D = D
        self._N = N
        self._barN = barN
        self._alpha_0 = alpha_0
        self._sigma = sigma

        # init state
        self._size_estimates = None
        self._traj = []
        self._x_traj = []
        self._M_log_x_traj = []
        self._S_traj = []
        for k in xrange(0,D):
            self._x_traj.append([])
            self._M_log_x_traj.append([])
            self._S_traj.append([])

        self._test_info = None
        
        self._epoch = 0

#        if self._epoch in logstats_at_epoch.keys():
#            self._state = AGENT_STATE_STARTINGUP
#        else
#            self._state = AGENT_STATE_SLEEPING

        self._state = AGENT_STATE_STARTINGUP

        self._sync_packets_at_epoch = sync_packets_at_epoch
        self._data_packets_at_epoch = data_packets_at_epoch
        self._logstats_at_epoch = logstats_at_epoch

        #
        # we are initialized
        #
        self._initialized = True



    def _compute_X_S_at_end_of_epoch(self, t):
        assert t in self._logstats_at_epoch.keys()

        X = numpy.array(self._logstats_at_epoch[t])
        S = float(self._M)/X
        return (X,S)


    def tick_epoch(self):
        assert len(self._traj) <= self._N + 1

        self._epoch +=1

        if self._epoch not in self._logstats_at_epoch.keys():
            #
            # we don't have any data for this epoch: we are sleeping
            #
            if not self._state == AGENT_STATE_SLEEPING:
                self._sleep()
            return
        
        if self._state == AGENT_STATE_SLEEPING:
            #
            # if we got here we aren't sleeping anymore
            #
            self._wake()


        self._test_info = None

        assert len(self._x_traj) == self._D
        assert len(self._M_log_x_traj) == self._D
        assert len(self._S_traj) == self._D

        X, S = self._compute_X_S_at_end_of_epoch(self._epoch)
        self._traj.append((self._epoch, X, S))

        for k in xrange(0,self._D):
            self._x_traj[k].append(X[k])
            self._M_log_x_traj[k].append(self._M*numpy.log(X[k]))
            self._S_traj[k].append(S[k])

        if len(self._traj) > self._N + 1:
            assert len(self._traj) == self._N+2

            # track only X(t-N),...,X(t)
            self._traj.pop(0)
            for k in xrange(0,self._D):
                assert len(self._x_traj[k]) == self._N+2
                assert len(self._M_log_x_traj[k]) == self._N+2
                assert len(self._S_traj[k]) == self._N+2
                self._x_traj[k].pop(0)
                self._M_log_x_traj[k].pop(0)
                self._S_traj[k].pop(0)
        else:
            if len(self._traj) < self._N + 1:
                assert self._state == AGENT_STATE_STARTINGUP

        if len(self._traj) == self._N+1:
            if not self._state == AGENT_STATE_SS:
                assert self._state == AGENT_STATE_STARTINGUP
                self._state = AGENT_STATE_SS

            # GLR test
            change_time = []
            pre_change_size = []
            loglambda = []
            alarm = []
            
            for k in xrange(0,self._D):
                x_traj = numpy.array(self._x_traj[k], dtype = float)
                M_log_x_traj = numpy.array(self._M_log_x_traj[k], dtype = float)
                S_traj = numpy.array(self._S_traj[k], dtype = float)
                x_traj_cumsum = numpy.cumsum(x_traj)
                
                # find the optimal change time
                argminT = 1
                minLogLambda = 0.
                argminbarS = 0.
                for T in xrange(self._N-self._barN + 1, 0, -1):
                    barS = float(self._M*(self._N+1-T))/x_traj_cumsum[self._N-T]
                    sigma_barS = self._sigma*barS

                    M_log_sigma_barS_over_M_plus_1 = self._M*(numpy.log(sigma_barS)) + self._M__1_minus_logM__

                    # compute the GLR using f_k(t-T+1),...,f_k(t)
                    LogLambda = 0.
                    for idx in xrange(self._N-T+1,self._N+1):
                        if S_traj[idx] < sigma_barS:
                            LogLambda +=  M_log_sigma_barS_over_M_plus_1 - sigma_barS*x_traj[idx] + M_log_x_traj[idx]

                    if LogLambda <= minLogLambda:
                        minLogLambda = LogLambda
                        argminT = T
                        argminbarS = barS

                change_time.append(argminT)
                pre_change_size.append(argminbarS)
                loglambda.append(minLogLambda)

                if minLogLambda < self._log_lambdas[argminT-1]:
                    alarm.append(1)
                else:
                    alarm.append(0)

            self._test_info = (change_time, pre_change_size, loglambda, alarm)
         
    def traj(self):
        return self._traj

    def state(self):
        return self._traj[len(self._traj)-1]

    def state2(self):
        return self._state

    def _sleep(self):
        assert not self._state == AGENT_STATE_SLEEPING

        self._state = AGENT_STATE_SLEEPING
        self._test_info = None
        self._traj = []
        self._x_traj = []
        self._M_log_x_traj = []
        self._S_traj = []
        for k in xrange(0,self._D):
            self._x_traj.append([])
            self._M_log_x_traj.append([])
            self._S_traj.append([])

    def _wake(self):
        assert self._state == AGENT_STATE_SLEEPING
        self._state = AGENT_STATE_STARTINGUP
        self._test_info = None

    def get_estimates_traj(self, k):
        assert isinstance(k, int) and k >= 1 and k <= self._D
        return self._S_traj[k-1]
    
    def test_info(self):
        return self._test_info

    def get_epoch(self):
        return self._epoch

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

    def get_sigma(self):
        assert self._initialized
        return self._sigma

    def get_alpha0(self):
        assert self._initialized
        return self._alpha_0

    def get_data_packets(self, epoch):
        assert self._initialized
        if epoch in self._data_packets_at_epoch.keys():
            return self._data_packets_at_epoch[epoch]
        return []

    def get_sync_packets(self, epoch):
        assert self._initialized
        if epoch in self._sync_packets_at_epoch.keys():
            return self._sync_packets_at_epoch[epoch]
        return []

    def last_epoch(self):
        assert self._initialized
        return max(chain(self._logstats_at_epoch.keys(), self._sync_packets_at_epoch.keys(), self._data_packets_at_epoch.keys()))


