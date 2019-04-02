#! /usr/bin/env python3
#
# vmax_restapi.py - Sample script to query EMC VMAX Storage System using
#                   Unisphere for VMAX REST API
#
# Homepage: http://thobias.org/python
# Author  : Thobias Salazar Trevisan (thobias at thobias.org)
#
# Changelog (DD/MM/YYYY):
# 13/03/2017 - first version
#
##############################################################################
#
# The purpose of this script is to help people to get start VMAX automation
#
# This is a sample python script to gather information using
# VMAX REST API. You can download a zip file with the VMAX Rest API 
# documentation from your Unisphere at:
#
# https://{UNIVMAX_IP}:{UNIVMAX_PORT}/univmax/restapi/docs
#
# UNIVMAX_PORT - usually is port 8443
#
# #####################################################
# #                   Usage:                          #
# #####################################################
#
# prompt> ./vmax_restapi.py -h
# usage: vmax_restapi.py [-h] [--version] [--verbose]
#                        {list,listsg,srp,performance} ...
#
# Sample Script to collect information from Unisphere for VMAX REST API
#
# optional arguments:
#   -h, --help            show this help message and exit
#   --version             show program's version number and exit
#   --verbose, -v         verbose flag
#
# Commands:
#   {list,listsg,srp,performance}
#     list                List VMAXs arrays managed by Unisphere
#     listsg              List VMAX storage groups
#     srp                 List SRP information
#     performance         List VMAX Array performance
#
#     Example of use:
#         ./vmax_restapi.py list
#         ./vmax_restapi.py srp --sid xxxx
#         ./vmax_restapi.py performance --sid xxxx --hours 2
#         ./vmax_restapi.py -v performance
#         ./vmax_restapi.py performance -h
#
# #####################################################
# #                   Comands help                    #
# #####################################################
#
##### list
# prompt> ./vmax_restapi.py list -h
# usage: vmax_restapi.py list [-h]
# 
# optional arguments:
#   -h, --help  show this help message and exit
#
#
##### listsg
# prompt> ./vmax_restapi.py listsg -h 
# usage: vmax_restapi.py listsg [-h] --sid SID
#
# optional arguments:
#   -h, --help  show this help message and exit
#   --sid SID   Symmetrix ID
#
#
##### srp
# prompt> ./vmax_restapi.py srp -h
# usage: vmax_restapi.py srp [-h] --sid SID
#
# optional arguments:
#   -h, --help  show this help message and exit
#   --sid SID   Symmetrix ID
#
#
##### performance
# prompt> ./vmax_restapi.py performance -h
# usage: vmax_restapi.py performance [-h] --sid SID [--min MIN] [--hours HOURS]
#                                    [--days DAYS]
# 
# optional arguments:
#   -h, --help     show this help message and exit
#   --sid SID      Symmetrix ID
#   --min MIN      Minutes ago to gather performance information
#   --hours HOURS  Hours ago to gather performance information
#   --days DAYS    Days ago to gather performance information
#
# ########
# # Hint #
# ########
#
# Use '-v' option for verbose output, so you can see the details about
# data sent and received from VMAX Rest API.
#
##############################################################################

# Import python modules
import requests
import pprint
import datetime
import time
import json
import sys
import logging
import argparse

###########################
# Unisphere configuration #
###########################
UNISPHERE_SERVER   = ''
UNISPHERE_USER = ''
UNISPHERE_PASS = ''
UNISPHERE_REST_URL = 'https://' + UNISPHERE_SERVER + ':8443/univmax/restapi'

# Script version
VERSION = '1.0'


##############################################################################
# Parses the command line arguments
##############################################################################
def parse_parameters():
    # parent --sid option. used by subparser srp and performance
    sid_parser = argparse.ArgumentParser(add_help=False)
    sid_parser.add_argument('--sid', help='Symmetrix ID', required=True)

    # epilog message: Custom text after the help
    epilog = '''
    Example of use:
        %s list
        %s srp --sid xxxx
        %s performance --sid xxxx --hours 2
        %s -v performance
        %s performance -h
    ''' % (sys.argv[0], sys.argv[0], sys.argv[0], sys.argv[0], sys.argv[0])
    # Create the argparse object and define global options
    parser = argparse.ArgumentParser(description='Sample Script to collect \
information from Unisphere for VMAX REST API',
                                    formatter_class=argparse.RawDescriptionHelpFormatter,
                                    epilog=epilog)
    # Global options
    parser.add_argument('--version',
                        action='version',
                        version=VERSION)
    parser.add_argument('--verbose', '-v',
                        action='store_true',
                        help='verbose flag',
                        dest='verbose')
    subparsers = parser.add_subparsers(title='Commands')

    # list options
    list_parser = subparsers.add_parser('list',
                                        help='List VMAXs arrays managed by Unisphere')
    list_parser.set_defaults(func=list_vmax)

    # listsg options
    list_parser = subparsers.add_parser('listsg',
                                        help='List VMAX storage groups',
                                       parents=[sid_parser])
    list_parser.set_defaults(func=list_sg)

    # srp options
    srp_parser = subparsers.add_parser('srp',
                                       help='List SRP information',
                                       parents=[sid_parser])
    srp_parser.set_defaults(func=list_srp)

    # performance options
    performance_parser = subparsers.add_parser('performance',
                                               help='List VMAX Array performance',
                                               parents=[sid_parser])
    performance_parser.add_argument('--min',
                                    type=int,
                                    help='Minutes ago to gather performance information',
                                    dest='min')
    performance_parser.add_argument('--hours',
                                    type=int,
                                    help='Hours ago to gather performance information',
                                    dest='hours')
    performance_parser.add_argument('--days',
                                    type=int,
                                    help='Days ago to gather performance information',
                                    dest='days')
    performance_parser.set_defaults(func=array_performance)

    # If there is no parameter, print help
    if len(sys.argv) < 2:
        parser.print_help()
        sys.exit(0)

    return parser.parse_args()


##############################################################################
# Configure logging
##############################################################################
def setup_logging():
    logging.basicConfig(level=logging.DEBUG,
                        format='%(asctime)s - %(funcName)s - %(levelname)s - %(message)s',
                        datefmt='%m/%d/%Y %I:%M:%S %p')
    # By default requests writes log messages to console.
    # The following line configure it to only write messages if is
    # at least warnings
    logging.getLogger("requests").setLevel(logging.WARNING)
    # Disable InsecureRequestWarning
    from requests.packages.urllib3.exceptions import InsecureRequestWarning
    requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
    return logging.getLogger(__name__)


##############################################################################
# Print colored messages
# If exitcode is not zero, finish the script
##############################################################################
def msg(color, msg, exitcode=0):
    if   color == 'blue'    : color = '\033[1;34m'
    elif color == 'red'     : color = '\033[1;31m'
    elif color == 'green'   : color = '\033[0;32m'
    elif color == 'yellow'  : color = '\033[0;33m'
    elif color == 'p_color' : color = '\033[01;37;44m'
    elif color == 'nocolor' : color = '0'
    else:
        print("Error: msg function: invalid color:" , color)
        sys.exit(1)

    resetcolor ='\033[0m'

    print(color + msg + resetcolor)

    if exitcode:
        sys.exit(exitcode)


##############################################################################
# Return current timestamp in milliseconds in Unix epoch time format
##############################################################################
def epoch_time_ms_now():
    return int(time.time() * 1000)


##############################################################################
# Return timestamp in milliseconds in Unix epoch time format
# Parameter: Minutes ago. Default is 5 minutes
##############################################################################
def epoch_time_min_ago(minutes=5):
    return int(epoch_time_ms_now() - (60 * minutes * 1000))


###########################################################################
# Return timestamp in milliseconds in Unix epoch time format
# Parameter: Hours ago. Default is 1 hour
##############################################################################
def epoch_time_hours_ago(hours=1):
    return int(epoch_time_ms_now() - (hours * 3600 * 1000))


##############################################################################
# Return timestamp in milliseconds in Unix epoch time format
# Parameter: Days ago. Default is 1 day
##############################################################################
def epoch_time_days_ago(days=1):
    return int(epoch_time_ms_now() - (days * 24 * 3600 * 1000))


##############################################################################
# Receive a timestamp in Unix epoch time format and return it
# in a readable format
# timestamp = Unix epoch time in milliseconds
# strftime (%c) - Locale's appropriate date and time representation.
##############################################################################
def epoch_time_to_human(timestamp):
    return datetime.datetime.fromtimestamp(timestamp/1000).strftime('%c')


##############################################################################
# Perform a POST request to Unisphere REST API and return the response as
# json object
##############################################################################
def vmax_rest_post(rest_path, request_obj):
    log.debug("rest_path: " + rest_path)
    if not log.disabled:
        log.debug("request_obj:")
        pprint.pprint(request_obj)

    # config the rest URL
    rest_url  = UNISPHERE_REST_URL + rest_path
    log.debug(rest_url)
    # set header
    header = { 'content-type' : 'application/json',
               'accept'       : 'application/json' }

    # Post data
    try:
        response = requests.post(rest_url,
                                 data=json.dumps(request_obj),
                                 auth=(UNISPHERE_USER, UNISPHERE_PASS),
                                 headers=header,
                                 verify=False)
    except Exception as e:
        msg("red", str(e))
        msg("red", "Error to connect to Unisphere Rest API", 1)
    log.debug("Response code %s", response.status_code)

    # check response
    if response.status_code != 200:
        pprint.pprint(response.text)
        msg("red", "Error to perform POST operation on unisphere. "       +
                   "\nrest_path: " + rest_path                            +
                   "\nRest API status_code: " + str(response.status_code) +
                   "\nError text: " + str(response.text), 1) 
    if not log.disabled:
        log.debug("Rest API response:")
        pprint.pprint(json.loads(response.text))

    # Return json data
    return json.loads(response.text)


##############################################################################
# Perform a GET request to Unisphere REST API and return the response as
# json object
##############################################################################
def vmax_restapi_get(rest_path):
    log.debug("rest_path: " + rest_path)
    # config the rest URL
    rest_url  = UNISPHERE_REST_URL + rest_path
    # set header
    headers = { 'content-type' : 'application/json',
                'accept'       : 'application/json' }

    # Get data
    try:
        response = requests.get(rest_url,
                                auth=(UNISPHERE_USER, UNISPHERE_PASS),
                                headers=headers,
                                verify=False)
    except Exception as e:
        msg("red", str(e))
        msg("red", "Error to connect to Unisphere Rest API", 1)

    if response.status_code != 200:
        msg("red", "Error to perform GET operation on unisphere."         +
                   "\nrest_path: " + rest_path                            +
                   "\nRest API status_code: " + str(response.status_code) +
                   "\nError text: " + str(response.text), 1)
    if not log.disabled:
        log.debug("Rest API response:")
        pprint.pprint(json.loads(response.text))

    # Return json data
    return json.loads(response.text)


##############################################################################
# Get VMAX array details
##############################################################################
def list_vmax_details(sid):
    log.debug("Getting VMAX details for SID: " + sid)
    rest_path = "/system/symmetrix/" + sid
    response = vmax_restapi_get(rest_path)

    output  = "Local: " + str(response['symmetrix'][0]['local'])
    output += " Model: " + str(response['symmetrix'][0]['model'])
    output += " symmetrixId: " + str(response['symmetrix'][0]['symmetrixId'])
    output += " ucode: " + str(response['symmetrix'][0]['ucode'])
    msg("blue", output)


##############################################################################
# List all VMAX arrays managed by Unisphere
##############################################################################
def list_vmax(args):
    log.debug("Getting list of VMAXs")
    rest_path = "/system/symmetrix"
    response = vmax_restapi_get(rest_path)

    log.debug("Start getting VMAXs details")
    for sid in response['symmetrixId']:
        # Get vmax array details
        list_vmax_details(sid)


##############################################################################
# List storage groups on a specific VMAX
##############################################################################
def list_sg(args):
    log.debug("Getting the list storage groups on VMAX: %s", args.sid)
    rest_path = "/sloprovisioning/symmetrix/" + args.sid + "/storagegroup"
    response = vmax_restapi_get(rest_path)

    for sg in response['storageGroupId']:
        print(sg)


##############################################################################
# Get SRP details
##############################################################################
def list_srp_details(rest_path):
    log.debug("Getting SRP detail: " + rest_path)
    response = vmax_restapi_get(rest_path)

    srp        = response['srp'][0]['srpId']
    usable     = response['srp'][0]['total_usable_cap_gb'] / 1024
    allocated  = response['srp'][0]['total_allocated_cap_gb'] / 1024
    subscribed = response['srp'][0]['total_subscribed_cap_gb'] / 1024
    snapshot   = response['srp'][0]['total_snapshot_allocated_cap_gb'] / 1024
    free = usable - allocated
    print("")
    print("SRP: " + srp)
    print("Usable TB: " + str(round(usable, 1)))
    print("Allocated TB: " + str(round(allocated, 1)))
    print("Snapshot TB: " + str(round(snapshot, 1)))
    print("Free TB: " + str(round(free, 1)))
    print("Subscribed TB: " + str(round(subscribed, 1)))
    print("")


##############################################################################
# List all SRPs available
##############################################################################
def list_srp(args):
    log.debug("Getting list of SRPs")
    rest_path = "/sloprovisioning/symmetrix/" + str(args.sid) + "/srp"
    response = vmax_restapi_get(rest_path)

    log.debug("Start getting SRPs details")
    for srp in response['srpId']:
        # Get srp details
        list_srp_details(rest_path + "/" + srp)


##############################################################################
# Parse time parameteres and return timestamps in epoch time
##############################################################################
def get_timestamp(args):
    if args.min:
        if args.min < 4:
            msg("red", "Error: minutes must be greater than 5", 1)
        start_time = epoch_time_min_ago(args.min)
    elif args.hours:
        if args.hours < 1:
            msg("red", "Error: hours must be greater than 1", 1)
        start_time = epoch_time_hours_ago(args.hours)
    elif args.days:
        if args.days < 1:
            msg("red", "Error: days must be greater than 1", 1)
        start_time = epoch_time_days_ago(args.days)
    else:
        start_time = epoch_time_min_ago(5)

    log.debug("start_time:")
    log.debug(time.strftime("%d %b %Y %H:%M:%S +0000", \
                            time.localtime(start_time // 1000)))
    return(start_time)


##############################################################################
# Return payload for vmax array performance metrics
##############################################################################
def generate_array_performance_payload(sid, start_time):
    log.debug('Symmetrix ID: ' + str(sid))

    return {'symmetrixId': str(sid),
            "endDate": epoch_time_ms_now(),
            "startDate": start_time,
            "metrics": ["HostIOs",
                        "PercentReads",
                        "PercentWrites",
                        "PercentHit",
                        "HostMBs",
                        "ReadResponseTime",
                        "WriteResponseTime"],
            "dataFormat": "Average" }


##############################################################################
# Get VMAX array performance
##############################################################################
def array_performance(args):
    log.debug("vmax sid: " + args.sid)
    start_time = get_timestamp(args)

    rest_path = "/performance/Array/metrics"
    # Set the performance payload
    request_obj = generate_array_performance_payload(args.sid, start_time)

    log.debug("start_time: " + epoch_time_to_human(start_time))
    log.debug("end_time  : " + epoch_time_to_human(request_obj['endDate']))

    response = vmax_rest_post(rest_path, request_obj)

    for metrics in response["resultList"]["result"]:
        # convert timestamp from unix epoch time to human readable
        # easier to see the output
        data = epoch_time_to_human(metrics["timestamp"]) + " "
        for key, value in metrics.items():
            # append to data the key=value returned in result
            data += key+"="+str(value)+","
        # remove , from ends of string
        data = data[:-1]
        msg("blue", data)


##############################################################################
# Main function
##############################################################################
def main():
    global log
    global args

    # Create the log
    log = setup_logging()
    # Parser the command line
    args = parse_parameters()
    # check if we need verbose output
    if not args.verbose:
        log.disabled = True
    log.debug('CMD line args: %s',  vars(args))

    # Execute the funcion (command)
    args.func(args)


##############################################################################
# Run from command line
##############################################################################
if __name__ == '__main__':
    main()


# vim: ts=4

