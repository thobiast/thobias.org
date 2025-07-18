---
layout: post
title:  "Exploring OpenStack Router Gateway Ports with OVN"
date:   2025-07-12
---

<a href="https://www.openstack.org/">
  <img src="/assets/images/openstack.png" alt="OpenStack site" title="OpenStack" width="110">
</a>
<a href="https://www.ovn.org/">
  <img src="/assets/images/ovn-logo.png" alt="OVN (Open Virtual Network)" title="OVN (Open Virtual Network)" width="80">
</a>


* This line is needed, but won't appear.
{:toc}


## Introduction

Neutron is the software-defined networking (SDN) component of OpenStack. Its main elements are the
API server, the Modular Layer 2 (ML2) plugin and agents, and the message queue.

Open Virtual Network (OVN) is currently the recommended ML2 Neutron driver for modern OpenStack deployments.
This post shows, how to query OVN databases to diagnose OpenStack router scheduling and external-gateway placement.


## Listing OVN Chassis (hosts) eligible for external-gateway ports

You can list chassis configured to handle external gateway ports with:

```bash
$ sudo ovn-sbctl --no-leader-only list Chassis | sed -n '/./{H;$!d}; x; /enable-chassis-as-gw/p'
```

The `list Chassis` sub‑command prints every chassis record as a paragraph, with blank lines acting as separators.
The sed command above extracts text blocks (paragraph) containing enable-chassis-as-gw.
Alternatively, you can achieve similar filter with awk:

```bash
$ sudo ovn-sbctl --no-leader-only list Chassis |  awk -v RS='' -v ORS='\n\n' '/enable-chassis-as-gw/'
```

To identify chassis configured with the new enable-chassis-as-extport-host OVN CMS option:

```bash
$ sudo ovn-sbctl --no-leader-only list Chassis | sed -n '/./{H;$!d}; x; /enable-chassis-as-extport-host/p'
```

Further reading:

- <https://bugs.launchpad.net/neutron/+bug/2037294>
- <https://docs.openstack.org/neutron/latest/admin/ovn/external_ports.html>


## Checking chassis priority for a router gateway ports

First, find the router's gateway port ID in OpenStack:

```bash
$ openstack port list --long --device-owner 'network:router_gateway' --router my-router
+--------------------------------------+------+-------------------+---------------------------------------+--------+-----------------+------------------------+------+
| ID                                   | Name | MAC Address       | Fixed IP Addresses                    | Status | Security Groups | Device Owner           | Tags |
+--------------------------------------+------+-------------------+---------------------------------------+--------+-----------------+------------------------+------+
| cabdac37-1acb-4983-a853-4defbd606ca3 |      | 02:7b:a5:9c:e3:41 | ip_address='10.10.0.12', subnet_id='  | ACTIVE | None            | network:router_gateway |      |
|                                      |      |                   | e9475bee-f32e-4cb8-a8e9-37fc1853e121' |        |                 |                        |      |
+--------------------------------------+------+-------------------+---------------------------------------+--------+-----------------+------------------------+------+
```

OVN names logical router ports (LRP) using the format `lrp-<port-uuid>`,
where `<port-uuid>` is the Neutron port ID. To check chassis priority:

```bash
$ sudo ovn-nbctl lrp-get-gateway-chassis lrp-<port-uuid>
```

Example:

```bash
$ sudo ovn-nbctl lrp-get-gateway-chassis lrp-cabdac37-1acb-4983-a853-4defbd606ca3
lrp-cabdac37-1acb-4983-a853-4defbd606ca3_host1d     3
lrp-cabdac37-1acb-4983-a853-4defbd606ca3_host2d     2
lrp-cabdac37-1acb-4983-a853-4defbd606ca3_host3d     1
```

The highest numeric priority wins, so 3 > 2 > 1. In this case, "*host1d*" is expected to own the gateway unless it becomes unavailable.
If that chassis goes down, OVN automatically assigns the port to the next-highest priority chassis.

## Displaying detailed information for Logical Router Ports:

To display details of a specific LRP:

```bash
$ sudo ovn-nbctl --no-leader-only list Logical_Router_Port lrp-<port-uuid>
```

## Finding the host where the router's gateway port (used for external connectivity) is scheduled

#### 1) Get OpenStack router ID

```bash
$ openstack router show -c id -f value my-router
4f64ef51-7511-4ce5-acb7-45c59a099aa6
```

#### 2) Match OpenStack Router to OVN Router

List all routers in the OVN Northbound database:

```bash
$ sudo ovn-nbctl lr-list
1f432978-0af1-4109-8a20-5da8bdd16c67 (neutron-0dca291d-3f34-4165-aa22-d6d7d363c352)
940716f1-110a-471e-95de-587ec1fb17d1 (neutron-2bc6060d-55c1-4378-a64a-22cfb05271d0)
be476590-356a-45cf-82ca-9a0a53d3fb2f (neutron-4f64ef51-7511-4ce5-acb7-45c59a099aa6)
0ebdb758-fe07-4923-9b45-5a652c017e07 (neutron-60cb2df1-7640-4ff8-a763-69cfe2dcb3fd)
```

The Neutron-OVN driver names every logical router using the pattern `neutron-<openstack-router-uuid>`.
Running 'ovn-nbctl lr-list' therefore shows these two columns. Example:

```bash
$ sudo ovn-nbctl lr-list
1f432978-0af1-4109-8a20-5da8bdd16c67 (neutron-0dca291d-3f34-4165-aa22-d6d7d363c352)
...
```

- 1st column : OVN's internal logical-router UUID
- 2nd column : `neutron-<openstack-router-uuid>`

#### 3) List the router's ports and locate the gateway port for our OpenStack router

Verify the external gateway port of the OpenStack router identified in Step 1:

```bash
$ sudo ovn-nbctl show neutron-4f64ef51-7511-4ce5-acb7-45c59a099aa6
router be476590-356a-45cf-82ca-9a0a53d3fb2f (neutron-4f64ef51-7511-4ce5-acb7-45c59a099aa6) (aka my-router)
    port lrp-ce1d8ecb-8f5d-4f09-b4b3-158232327b2a
        mac: "02:16:3e:65:5c:6a"
        networks: ["10.10.78.1/23"]
    port lrp-afbe083e-b5db-43a8-8f5e-6eb624263a30
        mac: "02:16:3e:29:42:66"
        networks: ["10.10.72.1/23"]
    port lrp-cabdac37-1acb-4983-a853-4defbd606ca3
        mac: "02:7b:a5:9c:e3:41"
        networks: ["10.10.0.12/24"]
        gateway chassis: [host1d host2d host3d]
    port lrp-4c527579-1559-4907-8245-a6b3f37a22d8
        mac: "02:16:3e:a8:4d:47"
        networks: ["10.10.76.1/23"]
    port lrp-c25bf513-61f0-4bfc-8e37-57a506e549b2
        mac: "02:16:3e:ac:8a:cd"
        networks: ["10.10.74.1/23"]
```

The gateway port will have the `gateway chassis` field.
Alternatively, you can also identify the gateway port using OpenStack CLI command:

```bash
$ openstack port list --long --device-owner 'network:router_gateway' --router my-router
+--------------------------------------+------+-------------------+---------------------------------------+--------+-----------------+------------------------+------+
| ID                                   | Name | MAC Address       | Fixed IP Addresses                    | Status | Security Groups | Device Owner           | Tags |
+--------------------------------------+------+-------------------+---------------------------------------+--------+-----------------+------------------------+------+
| cabdac37-1acb-4983-a853-4defbd606ca3 |      | 02:7b:a5:9c:e3:41 | ip_address='10.10.0.12', subnet_id='  | ACTIVE | None            | network:router_gateway |      |
|                                      |      |                   | e9475bee-f32e-4cb8-a8e9-37fc1853e121' |        |                 |                        |      |
+--------------------------------------+------+-------------------+---------------------------------------+--------+-----------------+------------------------+------+
```

`cabdac37-1acb-4983-a853-4defbd606ca3` is the ID of the router gateway port.

#### 4) List gateway chassis priority for that gateway port:

For the gateway port, each gateway chassis is configured with a priority. The gateway chassis with a high priority claims the gateway router port.
If that chassis goes down, OVN automatically assigns the port to the next-highest priority chassis.

Example to get chassis priority for port `cabdac37-1acb-4983-a853-4defbd606ca3`:

```bash
$ sudo ovn-nbctl lrp-get-gateway-chassis lrp-cabdac37-1acb-4983-a853-4defbd606ca3
lrp-cabdac37-1acb-4983-a853-4defbd606ca3_host1d    3
lrp-cabdac37-1acb-4983-a853-4defbd606ca3_host2d    2
lrp-cabdac37-1acb-4983-a853-4defbd606ca3_host3d    1
```

#### 5) Identify the chassis currently bound to the external port

There are three ways to find out to which chassis the external port is currently bound.

##### 5.1) Using ovn-sbctl show

```bash
$ sudo ovn-sbctl show | sed -n "/^Chassis/h; /lrp-cabdac37-1acb-4983-a853-4defbd606ca3/{x;p;x;p;}"
Chassis host1d
    Port_Binding cr-lrp-cabdac37-1acb-4983-a853-4defbd606ca3
```
The router gateway port is scheduled on chassis "*host1d*".

##### 5.2) Using chassis-redirect-port `cr‑lrp‑<uuid>`

For each gateway port, OVN automatically creates an internal chassis‑redirect port (`cr‑lrp‑<uuid>`),
which is the object actually bound to a chassis, and it uses the same UUID as the LRP.

```bash
$ sudo ovn-sbctl --no-leader-only --columns=logical_port,options list Port_Binding lrp-cabdac37-1acb-4983-a853-4defbd606ca3
logical_port        : lrp-cabdac37-1acb-4983-a853-4defbd606ca3
options             : {chassis-redirect-port=cr-lrp-cabdac37-1acb-4983-a853-4defbd606ca3, peer="cabdac37-1acb-4983-a853-4defbd606ca3"}
```

Verify chassis binding for the cr-lrp:

```bash
$ sudo ovn-sbctl --no-leader-only get Port_Binding cr-lrp-cabdac37-1acb-4983-a853-4defbd606ca3 chassis
82ede737-500c-4b56-8582-54e89023bc44

$ sudo ovn-sbctl get Chassis 82ede737-500c-4b56-8582-54e89023bc44 hostname
host1d
```

##### 5.3) Using OpenStack CLI

You can determine which host the port is bound to with OpenStack CLI command:

```bash
$ openstack port show -c binding_host_id cabdac37-1acb-4983-a853-4defbd606ca3
+-----------------+--------+
| Field           | Value  |
+-----------------+--------+
| binding_host_id | host1d |
+-----------------+--------+
```

## Finding the physical NIC that handles the external traffic


##### 1) Get the router's gateway port:

```bash
$ openstack port list --long --device-owner 'network:router_gateway' --router my-router
+--------------------------------------+------+-------------------+---------------------------------------+--------+-----------------+------------------------+------+
| ID                                   | Name | MAC Address       | Fixed IP Addresses                    | Status | Security Groups | Device Owner           | Tags |
+--------------------------------------+------+-------------------+---------------------------------------+--------+-----------------+------------------------+------+
| cabdac37-1acb-4983-a853-4defbd606ca3 |      | 02:7b:a5:9c:e3:41 | ip_address='10.10.0.12', subnet_id='  | ACTIVE | None            | network:router_gateway |      |
|                                      |      |                   | e9475bee-f32e-4cb8-a8e9-37fc1853e121' |        |                 |                        |      |
+--------------------------------------+------+-------------------+---------------------------------------+--------+-----------------+------------------------+------+
```

##### 2) Find the external network for that port

```bash
$ openstack port show -c network_id cabdac37-1acb-4983-a853-4defbd606ca3
+------------+--------------------------------------+
| Field      | Value                                |
+------------+--------------------------------------+
| network_id | 387ca03a-587f-40ed-98e1-1682dfd5b319 |
+------------+--------------------------------------+
```

##### 3) Identify the provider network (physnet) used by that network:

A provider network in OpenStack directly maps to a physical network in the data center infrastructure.


```bash
$ openstack network show -c provider:network_type -c provider:physical_network -c provider:segmentation_id 387ca03a-587f-40ed-98e1-1682dfd5b319
+---------------------------+----------+
| Field                     | Value    |
+---------------------------+----------+
| provider:network_type     | vlan     |
| provider:physical_network | physnet1 |
| provider:segmentation_id  | 2930     |
+---------------------------+----------+
```

"`physnet1`" is the provider (physnet) label for this external network.

##### 4) Map the physnet to its Open vSwitch (OVS) bridge on the binding chassis

`ovn-bridge-mappings` maps each physnet to an OVS bridge on each host.

Map using ovn-sbctl command:

```bash
$ sudo ovn-sbctl --no-leader-only get Chassis host1d other_config:ovn-bridge-mappings
"physnet1:br-ex,physnet2:br-pre"
```

In this example, that chassis has two provider (external) networks configured:

- `physnet1` that maps to OVS bridge `br-ex`
- `physnet2` that maps to OVS bridge `br-pre`

We saw earlier that the network we’re examining uses physnet1, which maps to the br‑ex bridge.

You can also map the physnet to its OVS bridge directly on the binding host ("*host1d*"):

```bash
root@host1d:~# ovs-vsctl get open_vswitch . external_ids:ovn-bridge-mappings
"physnet1:br-ex,physnet2:br-pre"
```

##### 5) List the physical NIC(s) behind that bridge

Run the following commands on the host chassis ("*host1d*" in this example).

List ports on the `br‑ex` bridge.

```bash
root@host1d:~# ovs-vsctl list-ports br-ex
external0
patch-provnet-3da825d6-a643-48c8-8a52-30238b2a71d6-to-br-int
patch-provnet-857f5656-512d-47e8-ab0e-4cc9423608e5-to-br-int
```

`patch-provnet-*` are internal to OVS and have no physical hardware behind them.
They connect `br‑ex` to `br‑int` for each provider network created by Neutron/OVN.

You can confirm that the `extern0` is the system interface with the command:

```bash
root@host1d:~# ovs-vsctl get interface external0 type
system

root@host1d:~# ip -c a show external0
13: external0: <BROADCAST,MULTICAST,MASTER,UP,LOWER_UP> mtu 9000 qdisc noqueue master ovs-system state UP group default qlen 1000
    link/ether 02:fa:12:34:56:78 brd ff:ff:ff:ff:ff:ff
    inet6 fe80::4fa:12ff:fe34:5678/64 scope link
       valid_lft forever preferred_lft forever

```

`external0` is the Linux network interface that carries the external (provider‑network) traffic for the router identified in Step 1.

For additional interface details:

```bash
root@host1d:~# ovs-vsctl list interface external0
...
```

List all bridges on the host:

```bash
root@host1d:~# ovs-vsctl list-br
br-ex
br-int
br-pre
```

## Automating these checks

I've created two helper scripts that gather the current gateway‑port binding:

- *openstack-router-port-gw-binding-list.sh* - uses only OpenStack CLI commands
- *local-ovn-router-gw-info.sh* -  uses `ovn-nbctl` and `ovn-sbctl` commands

Source code and usage instructions are available at
<https://github.com/thobiast/openstack-ovn-router-gw-info>.


## Useful links

- <https://docs.openstack.org/neutron/latest/admin>
- <https://docs.openstack.org/neutron/latest/install/index.html>
- <https://docs.openstack.org/neutron/latest/admin/ovn/index.html>
- <https://docs.ovn.org/en/latest/tutorials/ovn-openstack.html>
- <https://docs.openstack.org/openstack-ansible-os_neutron/latest/app-ovn.html>
- <https://docs.openstack.org/neutron/latest/admin/config-ml2.html>
- <https://docs.openstack.org/neutron/latest/admin/ovn/external_ports.html>
