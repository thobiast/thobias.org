#!/usr/bin/env python3
#
# vmax_tdev_pool_info.py - Script to query EMC VMAX Storage System using
#                          SYMCLI and print the LUNs space utilization
#                          per pool for a given Storage Group
#
# Homepage: http://thobias.org/python
# Author  : Thobias Salazar Trevisan (thobias at thobias.org)
#
# Changelog (DD/MM/YYYY):
# 15/10/2016 - first version
#
##############################################################################
#
# Prerequisite
# ============
# 
# EMC Solutions Enabler (SYMCLI)
#
# Documentation
# =============
#
# This script runs a few symcli list commands to gather information about
# LUNs pool utilization for the Storage Group specified at command line.
# Then, it parses the XML symcli output and create a python dictionary
# with the following structure:
#
# tdevs_dict = { tdev_name : {
#                           'total_tracks_gb' = LUN SIZE (GB)
#                           'alloc_tracks_gb' = LUN used space (GB)
#                           'alloc_percent'   = LUN used space (pct)
#                           'sg_parent'       = Storage Group Parent Name
#                           'sg_child'        = Storage Group Child Name
#                           pool_name : {
#                                        'alloc_tracks_gb'    = LUN used
#                                                               space in pool
#                                        'pool_alloc_percent' = LUN used
#                                                               space (PCT)
#                                       }
#                              }
#              }
#
# The prettytable module is used to print the dictionary information
# as a table.
#
# #####################################################
# #                   Usage:                          #
# #####################################################
#
# prompt> ./vmax_tdev_pool_info.py -h
# usage: vmax_tdev_pool_info.py [-h] [--version] [--verbose] [--nocolor] -sid
#                               SID
#                               SG_NAME
#
# Script to show LUNs space utilization per pool
#
# positional arguments:
#   SG_NAME        Storage Group Name to show LUNs space utilization
#
# optional arguments:
#   -h, --help     show this help message and exit
#   --version      show program's version number and exit
#   --verbose, -v  verbose flag
#   --nocolor      no color flag
#   -sid SID       Symmetrix ID
#
#     Example of use:
#         ./vmax_tdev_pool_info.py -sid 001 STORAGE_GROUP_1
#         ./vmax_tdev_pool_info.py -sid 002 STORAGE_GROUP_2
#
##############################################################################

# Import python modules
import logging
import pprint
import collections
import prettytable
import sys
import argparse
import subprocess
import xml.etree.ElementTree as ET

VERSION = '1.0'
COLORS = 1


##############################################################################
# Configure logging
##############################################################################
def setup_logging():
    logging.basicConfig(level=logging.DEBUG,
                        format='%(asctime)s - %(module)s - %(funcName)s -\
 %(levelname)s - %(message)s',
                        datefmt='%m/%d/%Y %I:%M:%S %p')
    return logging.getLogger(__name__)


##############################################################################
# Execute the command and return the output
# If command return code is != 0, terminate the script
##############################################################################
def run_command(cmd):
    log.debug("Executing command: %s", cmd)

    p = subprocess.Popen(cmd,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        universal_newlines=True,
                        shell=True)
    stdout, stderr = p.communicate()
    if p.returncode:
        message("red", "Error running command: " + cmd, 0)
        message("red", stderr, 1)
    return stdout


##############################################################################
# Print colored messages
# If exitcode parameter is not zero, finish the script
##############################################################################
def message(color, msg, exitcode=0):
    if   color == 'blue'    : color = '\033[1;34m'
    elif color == 'red'     : color = '\033[1;31m'
    elif color == 'green'   : color = '\033[0;32m'
    elif color == 'yellow'  : color = '\033[0;33m'
    elif color == 'p_color' : color = '\033[01;37;44m'
    elif color == 'nocolor' : color = '\033[0m'
    else:
        print("Error: message function: invalid color:" , color)
        sys.exit(1)

    nocolor ='\033[0m'

    if COLORS:
        print(color + msg + nocolor)
    else:
        print(msg)

    if exitcode:
        sys.exit(exitcode)


##############################################################################
# Create nested dic
##############################################################################
def nested_dict():
    return collections.defaultdict(nested_dict)


##############################################################################
# Return the text for a specific XML tag
##############################################################################
def get_xml_text(elem, tag):
    try:
        x = elem.find(tag).text
    except:
        # print error message and exit
        message("red", "Error: XML tag '" + tag + "' not found", 1)
    return x


##############################################################################
# Return list with VMAX pools
##############################################################################
def get_srp_pools(symmid):
    log.debug("Finding VMAX pools:")
    pools = list()
    xml_string = run_command('symcfg -sid '+symmid+' list -pool -output xml_e')
    pooltree = ET.fromstring(xml_string)
    for pool in pooltree.findall('Symmetrix/DevicePool/pool_name'):
        pools.append(pool.text)
    log.debug("Pools available: %s", pools)
    return pools


##############################################################################
# For all luns in tdevs_dict dictionary, get the storage group it belongs
##############################################################################
def get_tdevs_sg(symmid, tdevs_dict):
    log.debug("Getting storage group for tdev")
    for lun in tdevs_dict:
        xml_string = run_command('symaccess -sid '                        +
                                 symmid                                   +
                                 ' list -type storage -v -output xml_e -dev '+
                                 lun)
        luntree = ET.fromstring(xml_string)
        for elem in luntree.iterfind('Symmetrix/Storage_Group'):
            sg = get_xml_text(elem, 'Group_Info/group_name')
            status = get_xml_text(elem, 'Group_Info/Status')
            if status == 'IsParent':
                tdevs_dict[lun]['sg_parent'] = sg
            elif status == 'IsChild':
                tdevs_dict[lun]['sg_child']  = sg
            elif status == 'N/A':
                tdevs_dict[lun]['sg_parent'] = sg
                tdevs_dict[lun]['sg_child']  = 'N/A'
            else:
                message("red",
                        "Error: get_tdevs_sg. Luns is not parent, child or N/A "+
                            lun  +
                            " "  +
                            sg   +
                            " "  +
                            status,
                        1)


##############################################################################
# Return tdevs_dict dictionary data structure filled in
##############################################################################
def find_tdev_pool_alloc(symmid, sgname):
    log.debug("symmmid: %s and sgname: %s", symmid, sgname)
    # Find all VMAX pools
    pools = get_srp_pools(symmid)
    # SYMCLI command that return space pool allocation for all TDEVs
    # for the storage group 'sgname'
    xml_string = run_command('symcfg -sid '     +
                             symmid             +
                             ' list -tdev -sg ' +
                             sgname             +
                             ' -tb -detail -output xml_e')
    tdevs_dict = nested_dict()
    tdevtree = ET.fromstring(xml_string)
    for elem in tdevtree.iterfind('Symmetrix/ThinDevs/Device'):
        tdev_name = get_xml_text(elem, 'dev_name')
        log.debug("LUN: %s", tdev_name)
        tdevs_dict[tdev_name]['total_tracks_gb'] = get_xml_text(elem, 'total_tracks_gb')
        tdevs_dict[tdev_name]['alloc_tracks_gb'] = get_xml_text(elem, 'alloc_tracks_gb')
        try:
            tdevs_dict[tdev_name]['alloc_percent' ] = \
                int((float(tdevs_dict[tdev_name]['alloc_tracks_gb']) * 100) / \
                    float(tdevs_dict[tdev_name]['total_tracks_gb']) \
                   )
        except ZeroDivisionError:
            tdevs_dict[tdev_name]['alloc_percent' ] = "0.0"
        for pool in elem.iterfind('pool'):
            pool_name = get_xml_text(pool, 'pool_name')
            if pool_name == 'N/A':
                continue
            tdevs_dict[tdev_name][pool_name]['alloc_tracks_gb'] = \
                                    get_xml_text(pool, 'alloc_tracks_gb')
            tdevs_dict[tdev_name][pool_name]['pool_alloc_percent'] = \
                                    get_xml_text(pool, 'pool_alloc_percent')
        # want to be sure that tdev have information for all
        # pools available
        for pool_name in pools:
            if not tdevs_dict[tdev_name][pool_name]:
                tdevs_dict[tdev_name][pool_name]['alloc_tracks_gb'] = '0'
                tdevs_dict[tdev_name][pool_name]['pool_alloc_percent'] = '00'

    # Find storage group each lun belongs to and
    # the Storage Group Parent or Child
    get_tdevs_sg(symmid, tdevs_dict)

    if not log.disabled:
        log.debug("tdevs_dict:")
        pprint.pprint(tdevs_dict)

    return (pools, tdevs_dict)


##############################################################################
# Print table with lun space utilization per pool
##############################################################################
def print_tdevs(pools, tdevs):
    header = list()
    header.append("TDEV")
    header.append("Total GB")
    header.append("Alloc GB")
    header.append("Alloc Pct")
    for pool in sorted(pools):
        header.append("Alloc GB (" + pool + ")")
    header.append("SG Parent")
    header.append("SG Child")

    output = prettytable.PrettyTable(header)
    output.format = True
    for tdev in sorted(tdevs):
        row = list()
        row.append(tdev)
        row.append(tdevs[tdev]['total_tracks_gb'])
        row.append(tdevs[tdev]['alloc_tracks_gb'])
        row.append(tdevs[tdev]['alloc_percent'])
        for pool in sorted(pools):
            row.append(tdevs[tdev][pool]['alloc_tracks_gb']    +
                       " ("                                    +
                       tdevs[tdev][pool]['pool_alloc_percent'] +
                       "%)")
        row.append(tdevs[tdev]['sg_parent'])
        row.append(tdevs[tdev]['sg_child'])
        output.add_row(row)
    output.align["SG Child"] = "l"
    output.sortby = "SG Child"
    print(output)


##############################################################################
# Parses the command line arguments
##############################################################################
def parse_parameters():
    # epilog message: Custom text after the help
    epilog = '''
    Example of use:
        %s -sid 001 STORAGE_GROUP_1
        %s -sid 002 STORAGE_GROUP_2
    ''' % (sys.argv[0],sys.argv[0])
    # Create the argparse object and define global options
    parser = argparse.ArgumentParser(description='Script to show LUNs space \
utilization per pool',
                                    formatter_class=argparse.RawDescriptionHelpFormatter,
                                    epilog=epilog)
    parser.add_argument('--version',
                        action='version',
                        version=VERSION)
    parser.add_argument('--verbose', '-v',
                        action='store_true',
                        help='verbose flag',
                        dest='verbose')
    parser.add_argument('--nocolor',
                        action='store_false',
                        help='no color flag',
                        dest='color')
    parser.add_argument('-sid',
                        type=str,
                        required=True,
                        help='Symmetrix ID')
    parser.add_argument('SG_NAME',
                        help='Storage Group Name to show LUNs space utilization')

    # If there is no parameter, print help
    if len(sys.argv) < 2:
        parser.print_help()
        sys.exit(0)

    return parser.parse_args()


##############################################################################
# Main function
##############################################################################
def main():
    global COLORS
    global log
    # Create the log
    log = setup_logging()

    # Parser the command line
    args = parse_parameters()

    # Check if we are running verbose. If not, disable logging
    if not args.verbose:
        log.disabled = True
    log.debug('parameters: %s',args)
    if not args.color:
        log.debug("Disabling colored output")
        COLORS = 0

    pools, tdevs_dict = find_tdev_pool_alloc(args.sid, args.SG_NAME.upper())
    print_tdevs(pools, tdevs_dict)


##############################################################################
# Run from command line
##############################################################################
if __name__ == '__main__':
    main()


# vim: ts=4
