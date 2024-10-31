---
layout: post
title:  "OpenStack Octavia - Understanding Listener Connection Limits"
date:   2024-10-28
---

<a href="https://www.openstack.org/">
  <img src="/assets/images/openstack.png" alt="OpenStack site" title="OpenStack" width="110">
</a>
<a href="https://docs.openstack.org/octavia">
  <img src="/assets/images/octavia.png" alt="OpenStack Octavia site" title="OpenStack Octavia" width="100">
</a>


## Introduction

Recently, while a team was running performance tests on their application, the
application's OpenStack load balancer began reporting HTTP 503 Service Unavailable errors.

In this OpenStack cloud, we use the Octavia Amphora provider driver. Checking the Amphorae instance,
I saw that CPU and memory utilization were low, with no more than 20% of resources being used.
However, upon accessing the Amphorae instance and reviewing the logs, I noticed several
"*Resumed frontend*" entries in the HAProxy logs related to the listener ID. Examining the
`haproxy.cfg` file, I found the frontend configured with a
[maxconn](https://www.haproxy.com/documentation/haproxy-runtime-api/reference/set-maxconn-frontend/)
limit of 50,000 connections.

```console
$ cat haproxy.cfg
...
frontend a9a3b69a-738f-4273-bd28-4878597074da
    maxconn 50000
...
```

The maxconn parameter in the `haproxy.cfg` file defines the maximum number of concurrent
connections allowed.

By running `openstack loadbalancer stats show`, I observed that the active connections
were exactly at 50,000, matching the maxconn value. This confirmed that the connection
limit was being reached during the performance tests.

```console
$ openstack loadbalancer stats show d3ac3b10-c238-493d-8bdf-bb4b0493c613
+--------------------+---------------+
| Field              | Value         |
+--------------------+---------------+
| active_connections | 50000         |
| bytes_in           | 1652422780640 |
| bytes_out          | 4096967198622 |
| request_errors     | 2             |
| total_connections  | 1144423877    |
+--------------------+---------------+
```

However, the listener did not have a configured connection limit defined:

```console
$ openstack loadbalancer listener show -c connection_limit a9a3b69a-738f-4273-bd28-4878597074da
+------------------+-------+
| Field            | Value |
+------------------+-------+
| connection_limit | -1    |
+------------------+-------+
```

I was curious: how was this 50,000 limit set?

## Understanding the Default Connection Limit

According to the [Octavia API Reference](https://docs.openstack.org/api-ref/load-balancer/v2/index.html#create-listener),
the *connection\_limit* option description:

> "*The maximum number of connections permitted for this listener. Default value is -1 which represents
> infinite connections or a default value defined by the provider driver.*"

This suggests that the connection limit might also be set by the
[provider drivers](https://docs.openstack.org/octavia/latest/admin/providers/index.html).
Since we use the Octavia Amphora driver, I researched further and found that, starting with
[OpenStack Victoria](https://docs.openstack.org/releasenotes/octavia/victoria.html#relnotes-7-0-0-unmaintained-victoria)
version, the default connection limit for Amphora provider was
[changed](https://review.opendev.org/c/openstack/octavia/+/735126) to 50,000.

Documentation:

- <https://docs.openstack.org/octavia/latest/configuration/configref.html#haproxy-amphora>

> default_connection_limit
> 
> Type: integer
> 
> Default: 50000
> 
> Default connection_limit for listeners, used when setting “-1” or when unsetting connection_limit with the listener API.

- <https://docs.openstack.org/releasenotes/octavia/victoria.html#relnotes-7-0-0-unmaintained-victoria>

> "*Add a new configuration option to define the default connection_limit for new listeners
> that use the Amphora provider. The option is [haproxy_amphora].default_connection_limit and
> its default value is 50,000. This value is used when creating or setting a listener with -1
> as connection_limit parameter, or when unsetting connection_limit parameter.*"

## Resolving the Connection Limit Issue

By adjusting the listener's `connection_limit` option, we resolved the HTTP 503 errors
during high-load testing.

Updating this option will vary depending on how the load balancer was deployed, whether using
[Openstack Octavia CLI](https://docs.openstack.org/python-octaviaclient/latest/cli/index.html#listener),
[terraform](https://registry.terraform.io/providers/terraform-provider-openstack/openstack/latest/docs/resources/lb_listener_v2),
[Kubernetes](https://github.com/kubernetes/cloud-provider-openstack/blob/master/docs/openstack-cloud-controller-manager/expose-applications-using-loadbalancer-type-service.md)
etc.

It's important to note that increasing the `connection_limit` can lead to higher CPU and
[memory utilization](https://review.opendev.org/c/openstack/octavia/+/735126)
on the Amphora instances. It’s a good idea to keep an eye on these resources to make sure
the load balancer stays stable with the new configuration.


## Additional documentation

- <https://docs.openstack.org/releasenotes/octavia/victoria.html>
- <https://docs.openstack.org/octavia/latest/admin/providers/index.html>
- <https://docs.openstack.org/api-ref/load-balancer/v2/index.html>
- <https://docs.openstack.org/octavia/latest/configuration/configref.html>
- <https://review.opendev.org/c/openstack/octavia/+/735126>

