---
layout: post
title:  "Unified OpenStack Load Balancer View with openstack-lb-info"
date:   2025-05-02
---

<a href="https://www.openstack.org/">
  <img src="/assets/images/openstack.png" alt="OpenStack site" title="OpenStack" width="110">
</a>
<a href="https://docs.openstack.org/octavia">
  <img src="/assets/images/octavia.png" alt="OpenStack Octavia site" title="OpenStack Octavia" width="100">
</a>


* This line is needed, but won't appear.
{:toc}


## Introduction

When we need an overall view of an OpenStack load balancer — or we’re troubleshooting one in an
ERROR or DEGRADED state — the usual approach is time-consuming and tedious.
Typically, it involves running a series of OpenStack CLI commands to piece together the status
and relationships of each component:

```console
$ openstack loadbalancer show <lb_id>
$ openstack loadbalancer listener list --loadbalancer <lb_id>
$ openstack loadbalancer listener show <listener_id>
$ openstack loadbalancer pool list --loadbalancer <lb_id>
$ openstack loadbalancer member list <pool_id>
$ openstack loadbalancer healthmonitor show <healthmonitor_id>
...
```

Because I’m often asked to help diagnose load balancer issues,
I built **`openstack-lb-info`**, a CLI tool that automates these checks and delivers a clear, consolidated
view with a single command.

## What is openstack-lb-info?

**`openstack-lb-info`** is a CLI utility that connects to an OpenStack cloud and retrieves information about
load balancers and their components such as listeners, pools, health monitors, members, and amphorae.

The script aggregates and presents the data in a clear, human-readable format,
giving you a quick, comprehensive overview of the load balancer.

It supports multiple output formats:

- Rich: a colorful, hierarchical tree view using the [Rich library](https://github.com/Textualize/rich)
- Plain text
- JSON

Example:

![screenshot](/assets/images/openstack-lb-info.png)

Find the code and instructions in the
[GitHub repository](https://github.com/thobiast/openstack-loadbalancer-info).

## Prometheus Monitoring

While **`openstack-lb-info`** provides a useful snapshot of a load balancer’s configuration and status,
for continuous visibility we should use
[Octavia's native Prometheus metrics export](https://docs.openstack.org/octavia/latest/user/guides/monitoring.html).

Starting with the [OpenStack Yoga release](https://docs.openstack.org/releasenotes/octavia/yoga.html),
Octavia introduced support for Prometheus listeners.
This feature lets us create a Prometheus listener to expose metrics about load balancers, listeners, pools, and members.
These metrics can then be scraped by Prometheus and integrated into systems like Grafana for automated alerting and dashboards.


## Additional documentation

- <https://docs.openstack.org/releasenotes/octavia/yoga.html>
- <https://docs.openstack.org/octavia/latest/user/guides/monitoring.html>
- <https://github.com/thobiast/openstack-loadbalancer-info>

