#! /usr/bin/python

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
import sys
import re
import time
import shutil
import subprocess

#sys.path.append(os.path.abspath('/math'))
#from fixpointops import *

import numpy

import senslab

import change_detection

EXPERIMENT_LOG_FILENAME = 'experiment.log'

def _vm_query_filenames(site, user):
    assert user is not None
    cmd = 'ssh %s@%s.senslab.info " find . -maxdepth 1 -type f -print0 | xargs -0 ls --format=single-column"' % (user, site)

    try:
        ret = subprocess.check_output(cmd, shell=True)
        return [os.path.basename(x) for x in str(ret).split('\n') if len(x.strip())>0]
    except:
        print 'error: could not ssh to the vm'
        return []


def _local_query_experiment_dirs(dirpath):

    if not os.path.isdir(dirpath):
        print "error: cannot list the folder %s" % dirpath
        return []
    
    names = []
    for dirname in os.listdir(dirpath):
        expdata_path = os.path.join(dirname,dirname)
        if not os.path.isdir(dirname):
            continue
        if os.path.isfile(expdata_path):
            names.append(dirname)
        
    return names


def _sort_experiments_from_filenames(filenames):
    assert filenames is not None

    if not len(filenames):
        return []

    experiments = []

    for f in filenames:
        m = re.search('^([0-9]+)\.(.+)' , f)
        if m is not None:
            # we have found a new experiment
            exp_id = int(m.group(1))
            exp_name = m.group(2)
            experiments.append((f, exp_id, exp_name))

    # sort experiments by their id: those with higher id first
    experiments = sorted(experiments, key=lambda entry : -entry[1])
    return [entry[0] for entry in experiments]


def vm_query_experiments_sorted(user):
    print 'Retrieving the list of experiments from %s\'s vm-home ...' % user
    return _sort_experiments_from_filenames(_vm_query_filenames(user))


def local_query_experiments_sorted(dirpath):
    return _sort_experiments_from_filenames(_local_query_experiment_dirs(dirpath))


if __name__ == "__main__":
    from optparse import OptionParser

    parser = OptionParser()
    parser.add_option("-l", "--local", dest="local",
                      help="load the experiment from the local path given in `filename`. If filename is `last` load the experiment with higher id.", metavar="filename")
    parser.add_option("-f", "--fetch", dest="fetch",
                      help="fetch the experiment log with the path given in `filename` from senslab's vm. If filename is `last` fetch the experiment with higher id.", metavar="filename")
    parser.add_option("-u", "--user", dest="user",
                      help="set the username of your senslab's ssh account. This is only necessary when fetching data from the vm.", metavar="user")
    parser.add_option("-s", "--site", dest="site",
                      help="set one of the following sites strasbourg (default), lille or grenoble. This is only necessary when fetching data from the vm.", metavar="site")
    parser.add_option("-o", "--output-split-logs", action="store_true", dest="output_split_logs", default=False,
                      help="output split-logs one for each node.")

    (options, args) = parser.parse_args()

    if len(args):
        print "error: cannot parse these arguments %s\n" % args
        parser.print_help()
        sys.exit(0)

    if not options.site:
        print "error: no senslab site given."
        sys.exit(0)

    if not senslab.siteinfo.exists(options.site):
        print "error: unknown senslab site \"%s\"." % options.site
        sys.exit(0)
        
    if not options.local and not options.fetch:
        options.local = 'last'
    elif options.local and options.fetch:
        print "warning: both --local and --fetch were passed, --local will take precedence\n"
        optiorn.fetch = None


    experiment_dir = None
    experiment_path = None
    if options.local:
        #
        # try to load the experiment data from a local folder
        #
        if options.local == 'last':
            experiments = local_query_experiments_sorted('./')
            if not len(experiments):
                print 'error: no local experiments found.'
                sys.exit(0)

            experiment_dir = experiments[0]
        else:    

            experiment_dir = options.local

        experiment_path = os.path.join(experiment_dir, EXPERIMENT_LOG_FILENAME)
        if not os.path.isfile(experiment_path):
            print 'error: cannot find local experiment %s' % options.local
            sys.exit(0)
        print "Loading experiment data from %s" % experiment_path
    else:
        #
        # fetch the experiment from the senslab vm
        #
        remote_experiment_filename = None
        if not options.user:
            print "error: cannot fetch experiments from the vm without the ssh username\n"
            parser.print_help()
            sys.exit(-1)

        if options.fetch == 'last':
            experiments = vm_query_experiments_sorted(options.site, options.user)
            if not len(experiments):
                print "error: no experiments found on vm.\n"
                sys.exit(0)
                
            remote_experiment_filename = experiments[0]
        else:
            remote_experiment_filename = options.fetch

        #
        # create a new folder for the experiment and fetch the data
        #
        experiment_dir = os.path.abspath(remote_experiment_filename)
        
        if os.path.exists(experiment_dir):
            answer = raw_input("An experiment with the same name exists locally. Do yout want to overwrite it ? [y/N] ")
            if answer.lower() == 'y':
                shutil.rmtree(experiment_dir)
            else:
                os.sys.exit(0)

        os.mkdir(experiment_dir)
        experiment_path = os.path.join(experiment_dir, EXPERIMENT_LOG_FILENAME)

        print "Fetching experiment \"%s\" from vm to %s ..." % (remote_experiment_filename, os.path.join(os.path.basename(experiment_dir), EXPERIMENT_LOG_FILENAME))
            
        try:
            cmd = 'scp %s@%s.senslab.info:~/%s %s' % (options.user, options.site, remote_experiment_filename, experiment_path)
            subprocess.check_call(cmd, shell=True)
            print ""
        except:
            print "error: cannot fetch the experiment."
            os.rmdir(experiment_dir)
            sys.exit(-1)

    assert(os.path.isfile(experiment_path))

    CHANGE_DETECTOR_N      = 25
    CHANGE_DETECTOR_BARN   = 10
    CHANGE_DETECTOR_ALPHA0 = 0.001
    CHANGE_DETECTOR_SIGMA  = 1.0

    network = change_detection.Network(options.site, experiment_path, CHANGE_DETECTOR_N, CHANGE_DETECTOR_BARN, CHANGE_DETECTOR_ALPHA0, CHANGE_DETECTOR_SIGMA, output_split_logs=options.output_split_logs)
    if not network.initialized():
        print "warning: cannot initialize network."
        sys.exit(0)

    network.draw_graph()

