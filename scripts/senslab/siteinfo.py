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

#
# A dictionary, keyed by the site name, whose values are `node-info` dictionaries 
#
# The `node-info` dictionary are keyed by the node-id/sensor number; for each
# node-id the corresponding `value` is a tuple with the node position in 3d space and
# the 16 bit board-id
#
_siteinfo = {}  # by node-id
_rev_siteinfo = {} # by board-id


def _site_csv_path(site):
	assert site is not None
	assert isinstance(site, str)

	data_dir, this_script = os.path.split(os.path.abspath(__file__))

	site_csv = ''.join((site, '.csv'))
	return os.path.join(data_dir, site_csv)


def _load_siteinfo(site):
	#
	# Check if we already loaded it
	#
	if site in _siteinfo.keys():
		return _siteinfo[site]
	
	#
	# Check if the `site.csv` exists
	#
	site_csv_path = _site_csv_path(site)
	if not os.path.isfile(site_csv_path):
		_siteinfo[site] = None
		return None

	#
	# Parse the csv
	#
	info = {}
	rev_info = {}

	lines = file(site_csv_path).readlines()

	assert lines[0].startswith(r'"nodeid","x","y","z","nodeuid"')
	lines.pop(0)

	for line in lines:
		line = line.rstrip('\n')
		m = re.search(r'([0-9]+),([0-9\.]+),([0-9\.]+),([0-9\.]+),"([0-9a-f]+)"$', line)
		if m is not None:
			nodeid = int(m.group(1))
			posx = float(m.group(2))
			posy = float(m.group(3))
			posz = float(m.group(4))
			boardid = int(m.group(5), 16)
			
			assert nodeid not in info.keys()
			info[nodeid] = ((posx, posy, posz), boardid)
			rev_info[boardid] = nodeid
			
	_siteinfo[site] = info
	_rev_siteinfo[site] = rev_info
	
	return info

    
def exists(site):
	if site in _siteinfo.keys():
		return True

	return os.path.isfile(_site_csv_path(site))


def get_boardid(site, nodeid):
	info = _load_siteinfo(site)

	if info is None or nodeid not in info.keys():
		return None
	
	return info[nodeid][1]


def get_position(site, nodeid):
	info = _load_siteinfo(site)

	if info is None or nodeid not in info.keys():
		return None

	return info[nodeid][0]


def get_nodeid_from_boardid(site, boardid):
	if not site in _rev_siteinfo.keys():
		if not _load_siteinfo(site):
			return None

	rev_info = _rev_siteinfo[site]
	try:
		return rev_info[boardid]
	except:
		return None
