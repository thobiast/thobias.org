#!/usr/bin/env python3
#
# vmax_sg_info.py - Script to query EMC VMAX Storage System and print
#                   useful information about Storage Groups
#
# Homepage: http://thobias.org/python
# Author  : Thobias Salazar Trevisan (thobias at thobias.org)
#
# Changelog (DD/MM/YYYY):
# 19/09/2016 - first version
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
# This script runs only the following symcli command:
# prompt>  symsg -sid XX list -v -output xml_e
# 
# PS: Using the option '-output xml_e' symcli generates XML output.
#
# The script run the symsg command and parse the XML output.
# There is a lot of useful information in that command output.
# After the parse, the script creates the following python
# dictionary structure:
#
# sgs = { 'sg_name' :
#                   { 'slo_name'   : SLO name
#                     'symmid'     : Symmetrix ID
#                     'srp_name'   : SRP name
#                     'workload'   : Workload
#                     'hostlimit'  : Host limit
#                     'iops'       : Host IOPS Limit
#                     'mbs'        : Host MB/s Limit
#                     'dynamic'    : Dynamic Distribution
#                     'mv'         : Masking view 'yes/no'
#                     'relation'   : Cascaded status
#                                    P = Parent
#                                    S = Standalone
#                                    C = Child
#                     'parent'     : Storage Group Parent
#                     'child'      : List of Storage Groups Childs
#                     'num_luns'   : Number of LUNs
#                     'total_size' : Sum of all LUNs space in GB
#                     'luns' : {
#                              lun_id: size_mb
#                            }
#                   }
#       }
#
# #####################################################
# #                   Usage:                          #
# #####################################################
#
# prompt> ./vmax_sg_info.py -h
# usage: vmax_sg_info.py [-h] [--version] [--verbose] -sid SID
#
# Script to show Storage Group details
#
# optional arguments:
#   -h, --help     show this help message and exit
#   --version      show program's version number and exit
#   --verbose, -v  verbose flag
#   -sid SID       Symmetrix ID
#
#     Example of use:
#         ./vmax_sg_info.py -sid 001
#         ./vmax_sg_info.py -v -sid 002
#
##############################################################################

# import python modules
import sys
import pprint
import logging
import collections
import argparse
import subprocess
import prettytable
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
# If exitcode is not zero, finish the script
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
        # print error message and exit if tag not found
        message("red", "Error: XML tag '" + tag + "' not found", 1)
    return x


##############################################################################
# Create and populate the Storage Group dictionary
# sgs dictionary data structure:
# sgs = { 'sg_name' :
#                   { slo_name   : slo_name
#                     symmid     : Symm_ID
#                     srp_name   : srp_name
#                     workload   : workload
#                     hostlimit  : hostlimit
#                     dynamic    : dynamic_distribution
#                     iops       : iops
#                     mbs        : mbs
#                     mv         : yes/no
#                     relation   : P, S or C
#                     parent     : sg_parent
#                     child      : [sg_childs,...]
#                     num_luns   : number_luns
#                     total_size : total_size_GB
#                     luns : {
#                              lun_id: size_mb
#                            }
#                   }
#       }
##############################################################################
def sg_to_dict(symmid):
    sgs = nested_dict()
    xml_string = run_command("symsg -sid " + symmid + " list -v -output xml_e")
    sgtree = ET.fromstring(xml_string)
    #for each SG
    for elem in sgtree.iterfind('SG'):
        # get Storage Group Name
        sg_name  = get_xml_text(elem, 'SG_Info/name')
        # get some SG informations
        sgs[sg_name]['symmid'] = get_xml_text(elem, 'SG_Info/symid')
        sgs[sg_name]['slo_name'] = get_xml_text(elem, 'SG_Info/SLO_name')
        sgs[sg_name]['workload'] = get_xml_text(elem, 'SG_Info/Workload')
        sgs[sg_name]['srp_name'] = get_xml_text(elem, 'SG_Info/SRP_name')
        sgs[sg_name]['hostlimit'] = get_xml_text(elem, 'SG_Info/HostIOLimit_status')
        sgs[sg_name]['dynamic'] = get_xml_text(elem, 'SG_Info/Dynamic_Distribution')
        sgs[sg_name]['iops'] = get_xml_text(elem, 'SG_Info/HostIOLimit_max_io_sec')
        sgs[sg_name]['mbs'] = get_xml_text(elem, 'SG_Info/HostIOLimit_max_mb_sec')
        sgs[sg_name]['mv'] = get_xml_text(elem, 'SG_Info/Masking_views')
        # initialize list of sgs parent and childs
        sgs[sg_name]['child'] = list()
        sgs[sg_name]['parent'] = list()

        # populate lists of SGs parent and childs
        for elem_sg_group in elem.iterfind('SG_Info/SG_group_info/SG'):
            relation     = get_xml_text(elem_sg_group, 'Cascade_Status')
            sg_group_name = get_xml_text(elem_sg_group, 'name')
            if relation == 'IsChild':
                sgs[sg_name]['child'].append(sg_group_name)
            elif relation == 'IsParent':
                sgs[sg_name]['parent'].append(sg_group_name)

        # Populate the 'relation' with S, P or C
        # S = standalone
        # P = Parent
        # C = Child
        if  not sgs[sg_name]['parent'] and not sgs[sg_name]['child']:
            sgs[sg_name]['relation'] = 'S'
        elif sgs[sg_name]['parent']:
            sgs[sg_name]['relation'] = 'C'
        elif sgs[sg_name]['child']:
            sgs[sg_name]['relation'] = 'P'

        # Populate the dictionary key 'luns' with lunid: its_size
        for lun in elem.iterfind('DEVS_List/Device'):
            lun_id = get_xml_text(lun, 'dev_name')
            lun_mb = get_xml_text(lun, 'megabytes')
            sgs[sg_name]['luns'][lun_id] = int(lun_mb) # store the size as int

        # Get the number of luns
        sgs[sg_name]['num_luns'] = len(sgs[sg_name]['luns'])
        # Sum all luns size and store value in GB
        sgs[sg_name]['total_size'] = sum(sgs[sg_name]['luns'].values()) // 1024

    if not log.disabled:
        log.debug("sg dict:")
        pprint.pprint(sgs)
    return sgs


##############################################################################
# Function to read the Storage Group dictionary and return a list
# with storage groups relationship.
# List format:
# [ ['sgpai','sgchild','sgchild'],['sgstandalone'],['sgpai','sgchild'],...]
##############################################################################
def get_sgs_relationship(sgs):
    sg_relation = list()
    for sg in sgs:
        sg_name = sg.title().upper() # get the SG (key) of the dict
        # SG is standalone
        if not sgs[sg]['parent'] and not sgs[sg]['child']:
            sg_relation.append([sg_name])
        # If SG does not have a Parent, ie, it is parent
        # so, get the list of childs
        elif not sgs[sg]['parent']:
            # convert the list of child to string
            x = ",".join(sgs[sg]['child'])
            # add the sg_name (parent) in the beginning of string
            x = sg_name + ',' + x
            # convert string to list
            sg_relation.append(x.split(","))
        else:
            pass

    if not log.disabled:
        log.debug("sg_relation list:")
        pprint.pprint(sg_relation)
    return sg_relation


##############################################################################
# Print the SG dictionary using PrettyTable library
##############################################################################
def print_sg_info(sgs_relationship, sgs):
    # Special color for Parent and Standalone SGs
    p_color = '\033[01;37;44m'
    nocolor = '\033[0m'

    header = list()
    header.append("SG_Name")
    header.append("Relation")
    header.append("SymmID")
    header.append("# LUNs")
    header.append("Total_Size (GB)")
    #header.append("SRP")
    header.append("SLO")
    header.append("Workload")
    header.append("Host_Limit")
    header.append("Host_Limit_IOPS")
    header.append("Host_Limit_MBs")
    header.append("Dynamic_Distr")
    header.append("Masking")

    output = prettytable.PrettyTable(header)
    output.format = True

    # each SG parent has a sub list with its children
    for i in sorted(sgs_relationship):
        # for each sg
        for sg_name in i:
            row = list()
            if sgs[sg_name]['relation'] == 'P' and COLORS:
                row.append(p_color + sg_name)
            elif sgs[sg_name]['relation'] == 'S' and COLORS:
                row.append(p_color + sg_name)
            else:
                row.append(sg_name)
            row.append(sgs[sg_name]['relation'])
            row.append(sgs[sg_name]['symmid'])
            row.append(str(sgs[sg_name]['num_luns']))
            row.append(str(sgs[sg_name]['total_size']))
            #row.append(sgs[sg_name]['srp_name'])
            row.append(sgs[sg_name]['slo_name'])
            row.append(sgs[sg_name]['workload'])
            row.append(sgs[sg_name]['hostlimit'])
            row.append(sgs[sg_name]['iops'])
            row.append(sgs[sg_name]['mbs'])
            row.append(sgs[sg_name]['dynamic'])
            if sgs[sg_name]['relation'] == 'P' and COLORS:
                row.append(sgs[sg_name]['mv'] + nocolor)
            elif sgs[sg_name]['relation'] == 'S' and COLORS:
                row.append(sgs[sg_name]['mv'] + nocolor)
            else:
                row.append(sgs[sg_name]['mv'])
            output.add_row(row)

    output.align["SG_Name"] = "l"
    print(output)


##############################################################################
# Parses the command line arguments
##############################################################################
def parse_parameters():
    # epilog message: Custom text after the help
    epilog = '''
    Example of use:
        %s -sid 001
        %s -v -sid 002
    ''' % (sys.argv[0],sys.argv[0])
    # Create the argparse object and define global options
    parser = argparse.ArgumentParser(description='Script to show Storage Group details',
                                    formatter_class=argparse.RawDescriptionHelpFormatter,
                                    epilog=epilog)
    parser.add_argument('--version',
                        action='version',
                        version=VERSION)
    parser.add_argument('--verbose', '-v',
                        action='store_true',
                        help='verbose flag',
                        dest='verbose')
    parser.add_argument('-sid',
                        type=str,
                        required=True,
                        help='Symmetrix ID')

    # If there is no parameter, print help
    if len(sys.argv) < 2:
        parser.print_help()
        sys.exit(0)

    return parser.parse_args()


##############################################################################
# Main function
##############################################################################
def main():
    global log

    # Create the log structure
    log = setup_logging()

    # Parser the command line
    args = parse_parameters()

    # check if we need verbose output
    if not args.verbose:
        log.disabled = True
    log.debug('CMD line args: %s', vars(args))

    if not args.sid.isdigit():
        message("red", "Error: SymmID must be a number", 1)

    sgs = sg_to_dict(args.sid)
    sgs_relationship = get_sgs_relationship(sgs)
    print_sg_info(sgs_relationship, sgs)


##############################################################################
# Run from command line
##############################################################################
if __name__ == '__main__':
    main()


# vim: ts=4
