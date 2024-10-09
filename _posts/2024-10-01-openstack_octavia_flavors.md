---
layout: post
title:  "OpenStack Octavia Flavors"
date:   2024-10-01
---

<a href="https://www.openstack.org/">
  <img src="/assets/images/openstack.png" alt="OpenStack site" title="OpenStack" width="110">
</a>
<a href="https://docs.openstack.org/octavia">
  <img src="/assets/images/octavia.png" alt="OpenStack Octavia site" title="OpenStack Octavia" width="100">
</a>

## Table of Contents

- [Introduction](#introduction)
  - [Providers and capabilities](#providers-and-capabilities)
- [Customizing Load Balancers with Octavia Flavors](#customizing-load-balancers-with-octavia-flavors)
  - [Creating a Flavor Profile](#creating-a-flavor-profile)
  - [Defining Flavors](#defining-flavors)
  - [Using Flavors to Create Load Balancers](#using-flavors-to-create-load-balancers)
- [More Customizations examples](#more-customization-examples)
  - [Custom Compute Flavors for Amphora Instances](#custom-compute-flavors-for-amphora-instances)
  - [Using Multiple Capabilities](#using-multiple-capabilities)
- [Additional documentation](#additional-documentation)


## Introduction

OpenStack Stein release introduced a new feature named [Octavia flavors](https://docs.openstack.org/releasenotes/octavia/stein.html#relnotes-4-0-0-stable-stein).
It allows administrators to define custom provider driver configurations, giving users flexibility when they are creating load balancers.

These customizations are defined per provider driver.
In this post, we will explore some customizations for the Amphora driver.

Octavia flavors enable users to create customized load balancer configurations that can be selected during the load balancer creation.
It allows users to choose different characteristics or features without requiring administrator intervention.

### Providers and capabilities

List all the Octavia provider drivers available in an OpenStack cloud:

```console
$ openstack loadbalancer provider list
+---------+----------------------------+
| name    | description                |
+---------+----------------------------+
| amphora | The Octavia Amphora driver |
| ovn     | Octavia OVN driver         |
+---------+----------------------------+
```

Verify which flavor capabilities (customizations) the amphora driver exposes:

```console
$ openstack loadbalancer provider capability list amphora
+-------------------+-----------------------+-------------------------------------------------------------------------------------+
| type              | name                  | description                                                                         |
+-------------------+-----------------------+-------------------------------------------------------------------------------------+
| flavor            | loadbalancer_topology | The load balancer topology. One of: SINGLE - One amphora per load balancer.         |
|                   |                       | ACTIVE_STANDBY - Two amphora per load balancer.                                     |
| flavor            | compute_flavor        | The compute driver flavor ID.                                                       |
| flavor            | amp_image_tag         | The amphora image tag.                                                              |
| availability_zone | compute_zone          | The compute availability zone.                                                      |
| availability_zone | management_network    | The management network ID for the amphora.                                          |
| availability_zone | valid_vip_networks    | List of network IDs that are allowed for VIP use. This overrides/replaces the list  |
|                   |                       | of allowed networks configured in `octavia.conf`.                                   |
+-------------------+-----------------------+-------------------------------------------------------------------------------------+
```

These capabilities are available to be defined/customized via Octavia flavors.


## Customizing Load Balancers with Octavia Flavors

### Creating a Flavor Profile

In order to create an Octavia flavor, it is first required to create a *flavor profile*.
The *flavor profile* defines the provider driver and the capabilities' customizations.

Example of creating a *flavor profile* to customize the *loadbalancer_topology* capability:

<u>Creating Single Topology Flavor Profile:</u>
```console
$ openstack loadbalancer flavorprofile create --name amphora-fp-single --provider amphora --flavor-data '{"loadbalancer_topology": "SINGLE"}'
+---------------+--------------------------------------+
| Field         | Value                                |
+---------------+--------------------------------------+
| id            | 82a5e9ef-bfce-48ea-afaf-c8e5c286e91e |
| name          | amphora-fp-single                    |
| provider_name | amphora                              |
| flavor_data   | {"loadbalancer_topology": "SINGLE"}  |
+---------------+--------------------------------------+
```

<u>Creating Active/Standby Topology Flavor Profile:</u>
```console
$ openstack loadbalancer flavorprofile create --name amphora-fp-active_standby --provider amphora --flavor-data '{"loadbalancer_topology": "ACTIVE_STANDBY"}'
+---------------+---------------------------------------------+
| Field         | Value                                       |
+---------------+---------------------------------------------+
| id            | ca605e43-ed96-4ac6-b6dd-b0bd4fbda054        |
| name          | amphora-fp-active_standby                   |
| provider_name | amphora                                     |
| flavor_data   | {"loadbalancer_topology": "ACTIVE_STANDBY"} |
+---------------+---------------------------------------------+
```

<u>Listing the Flavor profiles created:</u>
```console
$ openstack loadbalancer flavorprofile list
+--------------------------------------+---------------------------+---------------+
| id                                   | name                      | provider_name |
+--------------------------------------+---------------------------+---------------+
| 82a5e9ef-bfce-48ea-afaf-c8e5c286e91e | amphora-fp-single         | amphora       |
| ca605e43-ed96-4ac6-b6dd-b0bd4fbda054 | amphora-fp-active_standby | amphora       |
+--------------------------------------+---------------------------+---------------+
```

### Defining Flavors

After creating flavor profiles, we can create the Octavia flavor that will be visible for the users:

<u>Creating a Flavor for Single Topology:</u>
```console
$ openstack loadbalancer flavor create --name single-lb --flavorprofile amphora-fp-single --description "A single amphora" --enable
+-------------------+--------------------------------------+
| Field             | Value                                |
+-------------------+--------------------------------------+
| id                | 7fefa13e-0665-42d7-b54d-8a24aed2a633 |
| name              | single-lb                            |
| flavor_profile_id | 82a5e9ef-bfce-48ea-afaf-c8e5c286e91e |
| enabled           | True                                 |
| description       | A single amphora                     |
+-------------------+--------------------------------------+
```

<u>Creating a Flavor for Active/Standby Topology:</u>
```console
$ openstack loadbalancer flavor create --name active_standby-lb --flavorprofile amphora-fp-active_standby --description "A high-availability amphora" --enable
+-------------------+--------------------------------------+
| Field             | Value                                |
+-------------------+--------------------------------------+
| id                | 18c829b7-0901-46b0-86be-0b0264a0aa40 |
| name              | active_standby-lb                    |
| flavor_profile_id | ca605e43-ed96-4ac6-b6dd-b0bd4fbda054 |
| enabled           | True                                 |
| description       | A high-availability amphora          |
+-------------------+--------------------------------------+
```


### Using Flavors to Create Load Balancers

Users can now list all available flavors and choose the customization that meets their needs.

```console
$ openstack loadbalancer flavor list
+--------------------------------------+-------------------+--------------------------------------+---------+
| id                                   | name              | flavor_profile_id                    | enabled |
+--------------------------------------+-------------------+--------------------------------------+---------+
| 18c829b7-0901-46b0-86be-0b0264a0aa40 | active_standby-lb | ca605e43-ed96-4ac6-b6dd-b0bd4fbda054 | True    |
| 7fefa13e-0665-42d7-b54d-8a24aed2a633 | single-lb         | 82a5e9ef-bfce-48ea-afaf-c8e5c286e91e | True    |
+--------------------------------------+-------------------+--------------------------------------+---------+
```

<u>Create a load balancer using the single topology:</u>

```console
$ openstack loadbalancer create --name single-lb1 --flavor single-lb --vip-subnet-id private-subnet
...
```

<u>Create a load balancer using the active standby topology:</u>

```console
$ openstack loadbalancer create --name active_standby-lb1 --flavor active_standby-lb --vip-subnet-id private-subnet
...
```

## More Customization Examples

### Custom Compute Flavors for Amphora Instances

For example, the administrator can create Octavia flavors customizing the *compute_flavor* capability. It specifies
the [Nova compute flavor](https://docs.openstack.org/nova/latest/user/flavors.html) that will be used by the
Amphora instances, allowing them to adjust the CPU, memory, and disk resources allocated.
That way, users can, depending on their workload, pick a load balancer size that meets their requirements.

<u>Checking the Nova compute flavors available:</u>
```console
$ openstack flavor list
+--------------------------------------+-------+-------+------+-----------+-------+-----------+
| ID                                   | Name  |  RAM  | Disk | Ephemeral | VCPUs | Is Public |
+--------------------------------------+-------+-------+------+-----------+-------+-----------+
| b432e62e-3fbb-4590-94e0-555f12013533 | small | 1024  | 8    | 0         | 1     | True      |
| 80141a7c-3e21-4a2c-8733-f1ef720b63b7 | medium| 2048  | 8    | 0         | 2     | True      |
| f9f0c658-02bb-4d8b-b226-31aed0b6b6e2 | large | 4096  | 8    | 0         | 4     | True      |
+--------------------------------------+-------+-------+------+-----------+-------+-----------+
```

<u>Creating Octavia flavors profile:</u>
```console
$ openstack loadbalancer flavorprofile create --name fp.small  --provider amphora --flavor-data '{"compute_flavor": "b432e62e-3fbb-4590-94e0-555f12013533"}'
+---------------+------------------------------------------------------------+
| Field         | Value                                                      |
+---------------+------------------------------------------------------------+
| id            | 96a7b399-47ed-40c0-8de3-ece2a1b5553b                       |
| name          | fp.small                                                   |
| provider_name | amphora                                                    |
| flavor_data   | {"compute_flavor": "b432e62e-3fbb-4590-94e0-555f12013533"} |
+---------------+------------------------------------------------------------+

$ openstack loadbalancer flavorprofile create --name fp.medium --provider amphora --flavor-data '{"compute_flavor": "80141a7c-3e21-4a2c-8733-f1ef720b63b7"}'
+---------------+------------------------------------------------------------+
| Field         | Value                                                      |
+---------------+------------------------------------------------------------+
| id            | d406a26f-4bf5-42b7-a80f-b5f3069dee0e                       |
| name          | fp.medium                                                  |
| provider_name | amphora                                                    |
| flavor_data   | {"compute_flavor": "80141a7c-3e21-4a2c-8733-f1ef720b63b7"} |
+---------------+------------------------------------------------------------+

$ openstack loadbalancer flavorprofile create --name fp.large  --provider amphora --flavor-data '{"compute_flavor": "f9f0c658-02bb-4d8b-b226-31aed0b6b6e2"}'
+---------------+------------------------------------------------------------+
| Field         | Value                                                      |
+---------------+------------------------------------------------------------+
| id            | 45798216-7c69-43cb-ac64-c4762b3e18cb                       |
| name          | fp.large                                                   |
| provider_name | amphora                                                    |
| flavor_data   | {"compute_flavor": "f9f0c658-02bb-4d8b-b226-31aed0b6b6e2"} |
+---------------+------------------------------------------------------------+
```

<u>Creating Octavia flavors:</u>
```console
$ openstack loadbalancer flavor create --name small-lb  --flavorprofile fp.small  --description '1 VCPU(s), 1024 RAM, 8 Disk' --enable
+-------------------+--------------------------------------+
| Field             | Value                                |
+-------------------+--------------------------------------+
| id                | 11e638d5-c5ca-48f2-a901-f8112affb717 |
| name              | small-lb                             |
| flavor_profile_id | 96a7b399-47ed-40c0-8de3-ece2a1b5553b |
| enabled           | True                                 |
| description       | 1 VCPU(s), 1024 RAM, 8 Disk          |
+-------------------+--------------------------------------+

$ openstack loadbalancer flavor create --name medium-lb --flavorprofile fp.medium --description '2 VCPU(s), 2048 RAM, 8 Disk' --enable
+-------------------+--------------------------------------+
| Field             | Value                                |
+-------------------+--------------------------------------+
| id                | bdedd0cb-9b44-43b3-9d23-8e34594108e4 |
| name              | medium-lb                            |
| flavor_profile_id | d406a26f-4bf5-42b7-a80f-b5f3069dee0e |
| enabled           | True                                 |
| description       | 2 VCPU(s), 2048 RAM, 8 Disk          |
+-------------------+--------------------------------------+

$ openstack loadbalancer flavor create --name large-lb  --flavorprofile fp.large  --description '4 VCPU(s), 4096 RAM, 8 Disk' --enable
+-------------------+--------------------------------------+
| Field             | Value                                |
+-------------------+--------------------------------------+
| id                | 06d4eba8-3736-441b-9697-ec0724fe23aa |
| name              | large-lb                             |
| flavor_profile_id | 45798216-7c69-43cb-ac64-c4762b3e18cb |
| enabled           | True                                 |
| description       | 4 VCPU(s), 4096 RAM, 8 Disk          |
+-------------------+--------------------------------------+
```

<u>Listing Octavia flavors:</u>
```console
$ openstack loadbalancer flavor list
+--------------------------------------+-----------+--------------------------------------+---------+
| id                                   | name      | flavor_profile_id                    | enabled |
+--------------------------------------+-----------+--------------------------------------+---------+
| 06d4eba8-3736-441b-9697-ec0724fe23aa | large-lb  | 45798216-7c69-43cb-ac64-c4762b3e18cb | True    |
| 11e638d5-c5ca-48f2-a901-f8112affb717 | small-lb  | 96a7b399-47ed-40c0-8de3-ece2a1b5553b | True    |
| bdedd0cb-9b44-43b3-9d23-8e34594108e4 | medium-lb | d406a26f-4bf5-42b7-a80f-b5f3069dee0e | True    |
+--------------------------------------+-----------+--------------------------------------+---------+
```

Now, users can choose the load balancer size at creation time. For example, they can create a load balancer that uses the
"*large*" Nova compute flavor for the Amphora instance, allowing that load balancer to handle more requests than the "*small*" one.

```console
$ openstack loadbalancer create --name my-large-lb --flavor large-lb --vip-subnet-id private-subnet

```

### Using Multiple Capabilities

We can customize multiple capabilities in the same flavor profile.

<u>Creating a flavor profile with topology active/standby and different Nova flavors:</u>
```console
$ openstack loadbalancer flavorprofile create --name fp.active_standby.small  --provider amphora --flavor-data '{"loadbalancer_topology": "ACTIVE_STANDBY", "compute_flavor": "1"}'

$ openstack loadbalancer flavorprofile create --name fp.active_standby.medium --provider amphora --flavor-data '{"loadbalancer_topology": "ACTIVE_STANDBY", "compute_flavor": "2"}'

$ openstack loadbalancer flavorprofile create --name fp.active_standby.large  --provider amphora --flavor-data '{"loadbalancer_topology": "ACTIVE_STANDBY", "compute_flavor": "3"}'

```

You can combine any number of capabilities to create a flavor:

```console
$ openstack loadbalancer provider capability list amphora
+-------------------+-----------------------+-------------------------------------------------------------------------------------+
| type              | name                  | description                                                                         |
+-------------------+-----------------------+-------------------------------------------------------------------------------------+
| flavor            | loadbalancer_topology | The load balancer topology. One of: SINGLE - One amphora per load balancer.         |
|                   |                       | ACTIVE_STANDBY - Two amphora per load balancer.                                     |
| flavor            | compute_flavor        | The compute driver flavor ID.                                                       |
| flavor            | amp_image_tag         | The amphora image tag.                                                              |
| availability_zone | compute_zone          | The compute availability zone.                                                      |
| availability_zone | management_network    | The management network ID for the amphora.                                          |
| availability_zone | valid_vip_networks    | List of network IDs that are allowed for VIP use. This overrides/replaces the list  |
|                   |                       | of allowed networks configured in `octavia.conf`.                                   |
+-------------------+-----------------------+-------------------------------------------------------------------------------------+
```

Since the Octavia flavors are visible for users, it is important to choose clear and descriptive names for flavors
and also add a description to make it easy for users to choose the correct one for their needs.

## Additional documentation

- <https://docs.openstack.org/releasenotes/octavia/stein.html>
- <https://docs.openstack.org/octavia/latest/admin/flavors.html>
- <https://docs.openstack.org/octavia/latest/user/guides/basic-cookbook.html>

