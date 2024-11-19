---
layout: post
title:  "Auto Scaling OpenStack Instances Using CPU Metrics with Aodh, Heat, Ceilometer, and Gnocchi"
date:   2024-11-18
---

<a href="https://www.openstack.org/">
  <img src="/assets/images/openstack.png" alt="OpenStack site" title="OpenStack" width="110">
</a>


* This line is needed, but won't appear.
{:toc}

### 1. Introduction

In the OpenStack Rocky release, the `cpu_util` meter was
[deprecated](https://docs.openstack.org/releasenotes/ceilometer/rocky.html)
in Ceilometer and subsequently removed in the Stein release.
As a result, we now need to use the `cpu` metric in Heat templates for auto scaling.
The `cpu` metric is
[cumulative, increases over time, and is measured in nanoseconds (ns)](https://docs.openstack.org/ceilometer/latest/admin/telemetry-measurements.html).

To better understand this metric, let's start by reviewing some basic OpenStack metric commands.

### 2. Instance Metrics

#### Checking Available Metrics for an Instance

To list the metrics available for a specific instance, we can use the command:
"`openstack metric resource show <instance_uuid>`".
For example, to display the metrics for the instance with UUID `2c6797eb-d9ac-4ece-8b62-2fd93713544a`:

```bash
$ openstack metric resource show -c metrics -t instance 2c6797eb-d9ac-4ece-8b62-2fd93713544a
+---------+---------------------------------------------------------------------+
| Field   | Value                                                               |
+---------+---------------------------------------------------------------------+
| metrics | compute.instance.booting.time: 41c2ad16-68f0-47e6-af3e-8fc5b285b743 |
|         | cpu: 1b7755e0-5042-42ca-a04f-742a701dc30e                           |
|         | disk.ephemeral.size: 2c1341e6-e28f-47cc-af7a-17e10fadb522           |
|         | disk.root.size: c84252bb-ca9b-47fa-a46f-6d985704c2b9                |
|         | memory.resident: 2ded7cf8-cb27-481f-bcd8-2a55f7984193               |
|         | memory.swap.in: 5cd5be1f-9982-432b-8500-a1515bd7f071                |
|         | memory.swap.out: 27e9244d-c9ad-4d0d-8572-75b2f8b34520               |
|         | memory.usage: e6ea4bfb-2ebe-48a1-8fce-0114f3a7096c                  |
|         | memory: 006c8422-b5f2-4f3e-a3d7-00f7a258373e                        |
|         | vcpus: 8736272c-283d-4508-bb21-b0979418be14                         |
+---------+---------------------------------------------------------------------+
```

#### Viewing Metric Details

To view the details of the `cpu` metric for the instance, use:
"`openstack metric show --resource-id <instance_uuid> cpu`":

```bash
$ openstack metric show --resource-id 2c6797eb-d9ac-4ece-8b62-2fd93713544a cpu
+--------------------------------+-------------------------------------------------------------------+
| Field                          | Value                                                             |
+--------------------------------+-------------------------------------------------------------------+
| archive_policy/name            | ceilometer-low-rate                                               |
| creator                        | e1684780e5e543658ba24bd10bf30e23:e1ea3ce6647f4988ae264a6e81084c7a |
| id                             | 1b7755e0-5042-42ca-a04f-742a701dc30e                              |
| name                           | cpu                                                               |
| resource/created_by_project_id | e1ea3ce6647f4988ae264a6e81084c7a                                  |
| resource/created_by_user_id    | e1684780e5e543658ba24bd10bf30e23                                  |
| resource/creator               | e1684780e5e543658ba24bd10bf30e23:e1ea3ce6647f4988ae264a6e81084c7a |
| resource/ended_at              | None                                                              |
| resource/id                    | 2c6797eb-d9ac-4ece-8b62-2fd93713544a                              |
| resource/original_resource_id  | 2c6797eb-d9ac-4ece-8b62-2fd93713544a                              |
| resource/project_id            | e51c9ddf3b154e7da95ed0a8b4927601                                  |
| resource/revision_end          | None                                                              |
| resource/revision_start        | 2024-11-18T14:04:43.428610+00:00                                  |
| resource/started_at            | 2024-11-18T13:30:19.286339+00:00                                  |
| resource/type                  | instance                                                          |
| resource/user_id               | 008138dasda933e19f24c0e6aa19634c11366aa8b8as9adv830ad0ad9a2f961a  |
| unit                           | ns                                                                |
+--------------------------------+-------------------------------------------------------------------+
```

This output shows that the metric uses the unit "ns" (nanoseconds) and is associated
with the archive policy `ceilometer-low-rate`.

To inspect that archive policy, we can run the command:
"`openstack metric archive-policy show <archive_policy_name>`":


```bash
$ openstack metric archive-policy show ceilometer-low-rate
+---------------------+------------------------------------------------------------------+
| Field               | Value                                                            |
+---------------------+------------------------------------------------------------------+
| aggregation_methods | mean, rate:mean                                                  |
| back_window         | 0                                                                |
| definition          | - timespan: 30 days, 0:00:00, granularity: 0:05:00, points: 8640 |
| name                | ceilometer-low-rate                                              |
+---------------------+------------------------------------------------------------------+
```

The archive policy `ceilometer-low-rate` offers the aggregation methods `mean` and `rate:mean`,
with a granularity of 5 minutes (300 seconds). Granularity defines the time interval at
which the data points are stored.

In aggregation methods, `mean` calculates the average value, while `rate:mean` computes
the average rate of change over time; that is, it reflects the differences between
consecutive data points.

#### Querying CPU Metric Statistics

To retrieve CPU statistics for an instance, you can use either of the following commands:

- `openstack metric measures show --utc --aggregation mean -r <instance_uuid> cpu`
- `openstack metric aggregates --resource-type=instance '(metric cpu mean)' id=<instance_uuid>`

```bash
$ openstack metric measures show --utc --aggregation mean -r 2c6797eb-d9ac-4ece-8b62-2fd93713544a cpu
+---------------------------+-------------+------------------+
| timestamp                 | granularity |            value |
+---------------------------+-------------+------------------+
| 2024-11-18T13:30:00+00:00 |       300.0 |    97840000000.0 |
| 2024-11-18T13:35:00+00:00 |       300.0 |   309460000000.0 |
| 2024-11-18T13:40:00+00:00 |       300.0 |   908550000000.0 |
| 2024-11-18T13:45:00+00:00 |       300.0 |  1507590000000.0 |
| 2024-11-18T13:50:00+00:00 |       300.0 |  2108360000000.0 |


$ openstack metric aggregates --resource-type=instance '(metric cpu mean)' id=2c6797eb-d9ac-4ece-8b62-2fd93713544a
+-----------------------------------------------+---------------------------+-------------+------------------+
| name                                          | timestamp                 | granularity |            value |
+-----------------------------------------------+---------------------------+-------------+------------------+
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/mean | 2024-11-18T13:30:00+00:00 |       300.0 |    97840000000.0 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/mean | 2024-11-18T13:35:00+00:00 |       300.0 |   309460000000.0 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/mean | 2024-11-18T13:40:00+00:00 |       300.0 |   908550000000.0 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/mean | 2024-11-18T13:45:00+00:00 |       300.0 |  1507590000000.0 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/mean | 2024-11-18T13:50:00+00:00 |       300.0 |  2108360000000.0 |
```

Both commands will produce the same results.
The values represent cumulative CPU usage in nanoseconds.

You can specify the granularity using the `--granularity` option.
Note that if you specify a granularity that doesn't exist, you'll receive an error:

```bash
$ openstack metric aggregates --granularity 120 --resource-type=instance '(metric cpu mean)' id=2c6797eb-d9ac-4ece-8b62-2fd93713544a
{'cause': "Metrics can't being aggregated", 'reason': 'Granularities are missing', 'detail': [['cpu', 'mean', 120.0]]} (HTTP 400)
```

#### Understanding Aggregation Methods

Aggregation methods in Gnocchi determine how raw metric data is processed and summarized over
specified time intervals. For the `cpu` metric, it offers two aggregation methods:

- `mean`: Calculates the average value of the metric over the granularity period. For cumulative
metrics like `cpu`, this results in values that increase over time, reflecting the total CPU time
used up to each point.
- `rate:mean`: Computes the average rate of change between consecutive data points over
the granularity period.

Example of using the `rate:mean` aggregation method for the instance with UUID `2c6797eb-d9ac-4ece-8b62-2fd93713544a`.

```bash
$ openstack metric measures show --utc --aggregation rate:mean -r 2c6797eb-d9ac-4ece-8b62-2fd93713544a cpu
+---------------------------+-------------+---------------+
| timestamp                 | granularity |         value |
+---------------------------+-------------+---------------+
| 2024-11-18T13:35:00+00:00 |       300.0 | 211620000000.0 |
| 2024-11-18T13:40:00+00:00 |       300.0 | 599090000000.0 |
| 2024-11-18T13:45:00+00:00 |       300.0 | 599040000000.0 |
| 2024-11-18T13:50:00+00:00 |       300.0 | 600770000000.0 |


$ openstack metric aggregates --resource-type=instance '(metric cpu rate:mean)' id=2c6797eb-d9ac-4ece-8b62-2fd93713544a
+----------------------------------------------------+---------------------------+-------------+----------------+
| name                                               | timestamp                 | granularity |          value |
+----------------------------------------------------+---------------------------+-------------+----------------+
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:35:00+00:00 |       300.0 | 211620000000.0 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:40:00+00:00 |       300.0 | 599090000000.0 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:45:00+00:00 |       300.0 | 599040000000.0 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:50:00+00:00 |       300.0 | 600770000000.0 |
```

With `rate:mean`, the values represent the average rate of CPU usage between data points (in nanoseconds).
The first timestamp is omitted because there's no previous data point for comparison.

### 3. Calculating CPU Usage Percentage

When an instance uses one vCPU at 100% utilization for the entire granularity period,
it accumulates the maximum possible time of one CPU for that period.
In other words, a vCPU running at 100% is busy for every nanosecond of the granularity period.

> **Max CPU Time per vCPU (nanoseconds) = Granularity (in seconds) * Nanoseconds per Second**

An instance can consume as much CPU time as the number of vCPUs it has.
For example, consider an instance with 1 vCPU, a granularity of 300 seconds (5 minutes), and 100% utilization.
Formula to calculate the `rate:mean Value`:

> **rate:mean Value = Number of vCPUs * Granularity (seconds) * Nanoseconds per Second**

Applying the values to the formula:

> **rate:mean Value = 1 * 300 * 1000000000 = 300,000,000,000 ns**

The `rate:mean` value is 300,000,000,000 ns when the vCPU is fully utilized during the entire 300-second interval.

Therefore, to determine the CPU usage percentage of an instance, we can use the following formula:

> **CPU Usage (%) = ((value / 1,000,000,000 / granularity) * 100) / number_of_vCPUs**

Notes:
- `value`: The `rate:mean` value from the `cpu` metric (in nanoseconds).
- `1,000,000,000`: Nanoseconds per second.
- `granularity`: The Gnocchi granularity of the CPU data (in seconds).
- `number_of_vCPUs`: The number of vCPUs assigned to the instance.

Breaking it down:
- Divide the `rate:mean` value by `1,000,000,000` to convert nanoseconds to seconds.
- Divide by the granularity to get the average CPU time per second over the interval.
- Multiply by 100 to convert it into a percentage.
- Divide by the number of vCPUs to get the CPU usage percentage for the instance.

Alternative Formula:

> **CPU Usage (%) = (value / (number_of_vCPUs * granularity * 1,000,000,000)) * 100**

Suppose we have a `rate:mean` value of 600,000,000,000.0 ns for an instance with 2 vCPUs
and a granularity of 300 seconds:

```ts
CPU Usage (%) = ((600,000,000,000.0 / 1,000,000,000 / 300) * 100) / 2
              = ((600 / 300) * 100) / 2
              = (2 * 100) / 2
              = 200 / 2
              = 100%

// Alternative Formula:
CPU Usage (%) = (600,000,000,000 / (2 * 300 * 1,000,000,000)) * 100
              = (600,000,000,000 / 600,000,000,000) * 100
              = 1 * 100
              = 100%
```

The instance used 100% of each vCPU over the granularity period.

You can perform this calculation directly within the `openstack metric aggregates` command by using
mathematical expressions supported by Gnocchi.


```bash
$ openstack metric aggregates --resource-type instance --sort-column timestamp \
'(/ (* (/ (/ (metric cpu rate:mean) 1000000000) 300) 100) 2)' id=2c6797eb-d9ac-4ece-8b62-2fd93713544a
+----------------------------------------------------+---------------------------+-------------+--------------------+
| name                                               | timestamp                 | granularity |              value |
+----------------------------------------------------+---------------------------+-------------+--------------------+
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:35:00+00:00 |       300.0 |              35.27 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:40:00+00:00 |       300.0 |  99.84833333333334 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:45:00+00:00 |       300.0 |  99.83999999999999 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:50:00+00:00 |       300.0 | 100.12833333333333 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:55:00+00:00 |       300.0 | 100.07333333333335 |

$ # Alternative Formula
$ openstack metric aggregates --resource-type instance --sort-column timestamp \
'(* (/ (metric cpu rate:mean) (* 2 (* 300 1000000000))) 100)' id=2c6797eb-d9ac-4ece-8b62-2fd93713544a
+----------------------------------------------------+---------------------------+-------------+--------------------+
| name                                               | timestamp                 | granularity |              value |
+----------------------------------------------------+---------------------------+-------------+--------------------+
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:35:00+00:00 |       300.0 |              35.27 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:40:00+00:00 |       300.0 |  99.84833333333334 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:45:00+00:00 |       300.0 |  99.83999999999999 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:50:00+00:00 |       300.0 | 100.12833333333333 |
| 2c6797eb-d9ac-4ece-8b62-2fd93713544a/cpu/rate:mean | 2024-11-18T13:55:00+00:00 |       300.0 | 100.07333333333335 |
```

### 4. CPU threshold in Heat Auto Scaling Group

In Heat, you can configure an Aodh alarm using the resource type
[`OS::Aodh::GnocchiAggregationByResourcesAlarm`](https://docs.openstack.org/heat/latest/template_guide/openstack.html#OS::Aodh::GnocchiAggregationByResourcesAlarm).
Key configuration parameters:

- `metric`: The metric name, which should be `cpu`.
- `aggregation_method`: The method to compare to the threshold, which should be `rate:mean`.
- `threshold`: The calculated threshold value.


To calculate the CPU threshold for a given utilization percentage, use the following formula:

> **Threshold = Percentage_in_Decimal × Number_of_vCPUs × 1,000,000,000 × Granularity**

Notes:
- `Percentage_in_Decimal`: The desired CPU utilization percentage expressed as a decimal (e.g., 0.8 for 80%).
- `Number_of_vCPUs`: The number of vCPUs in the flavor used by the instance.
- `1,000,000,000`: Nanoseconds in one second.
- `Granularity`:  The granularity of the metric in seconds.


**<u>Example with 1 vCPU and a granularity of 300 seconds:</u>**

| Utilization (%) | Decimal_Value_of_Pct | Calculation | Threshold_Value |
| ---------------:| :-----------------:  | :---------: | ----: |
| 100% | 1   | 1   * 1 * 1000000000 * 300 | 300000000000.0 |
| 90%  | 0.9 | 0.9 * 1 * 1000000000 * 300 | 270000000000.0 |
| 80%  | 0.8 | 0.8 * 1 * 1000000000 * 300 | 240000000000.0 |
| 70%  | 0.7 | 0.7 * 1 * 1000000000 * 300 | 210000000000.0 |
| 60%  | 0.6 | 0.6 * 1 * 1000000000 * 300 | 180000000000.0 |
| 50%  | 0.5 | 0.5 * 1 * 1000000000 * 300 | 150000000000.0 |
| 40%  | 0.4 | 0.4 * 1 * 1000000000 * 300 | 120000000000.0 |
| 30%  | 0.3 | 0.3 * 1 * 1000000000 * 300 | 90000000000.0  |
| 20%  | 0.2 | 0.2 * 1 * 1000000000 * 300 | 60000000000.0  |
| 10%  | 0.1 | 0.1 * 1 * 1000000000 * 300 | 30000000000.0  |

**<u>Example with 2 vCPUs and a granularity of 300 seconds:</u>**

| Utilization (%) | Decimal_Value_of_Pct | Calculation | Threshold_Value |
| ---------------:| :-----------------:  | :---------: | ----: |
| 100% | 1   | 1   * 2 * 1000000000 * 300 | 600000000000.0 |
| 90%  | 0.9 | 0.9 * 2 * 1000000000 * 300 | 540000000000.0 |
| 80%  | 0.8 | 0.8 * 2 * 1000000000 * 300 | 480000000000.0 |
| 70%  | 0.7 | 0.7 * 2 * 1000000000 * 300 | 420000000000.0 |
| 60%  | 0.6 | 0.6 * 2 * 1000000000 * 300 | 360000000000.0 |
| 50%  | 0.5 | 0.5 * 2 * 1000000000 * 300 | 300000000000.0 |
| 40%  | 0.4 | 0.4 * 2 * 1000000000 * 300 | 240000000000.0 |
| 30%  | 0.3 | 0.3 * 2 * 1000000000 * 300 | 180000000000.0 |
| 20%  | 0.2 | 0.2 * 2 * 1000000000 * 300 | 120000000000.0 |
| 10%  | 0.1 | 0.1 * 2 * 1000000000 * 300 |  60000000000.0 |

**<u>Example with 4 vCPUs and a granularity of 300 seconds:</u>**

| Utilization (%) | Decimal_Value_of_Pct | Calculation | Threshold_Value |
| ---------------:| :-----------------:  | :---------: | ----: |
| 100% | 1   | 1   * 4 * 1000000000 * 300 | 1200000000000.0 |
| 90%  | 0.9 | 0.9 * 4 * 1000000000 * 300 | 1080000000000.0 |
| 80%  | 0.8 | 0.8 * 4 * 1000000000 * 300 |  960000000000.0 |
| 70%  | 0.7 | 0.7 * 4 * 1000000000 * 300 |  840000000000.0 |
| 60%  | 0.6 | 0.6 * 4 * 1000000000 * 300 |  720000000000.0 |
| 50%  | 0.5 | 0.5 * 4 * 1000000000 * 300 |  600000000000.0 |
| 40%  | 0.4 | 0.4 * 4 * 1000000000 * 300 |  480000000000.0 |
| 30%  | 0.3 | 0.3 * 4 * 1000000000 * 300 |  360000000000.0 |
| 20%  | 0.2 | 0.2 * 4 * 1000000000 * 300 |  240000000000.0 |
| 10%  | 0.1 | 0.1 * 4 * 1000000000 * 300 |  120000000000.0 |

Note: Adjust the calculations according to the number of vCPUs and granularity specific to your environment.


#### Sample Heat Template Snippet

```yaml
resources:
  asg:
    type: OS::Heat::AutoScalingGroup
    properties:
      desired_capacity: 1
      max_size: 3
      min_size: 1
      resource:
        type: template_server.yaml
        properties:
          flavor: {get_param: flavor}
          image: {get_param: image}
          key_name: {get_param: key_name}
          subnet: {get_param: subnet}
          metadata: {"metering.server_group": {get_param: "OS::stack_id"},
                     "stack_id": {get_param: "OS::stack_id"},
                     "stack_name": {get_param: "OS::stack_name"}}

  cpu_utilization_alarm_high:
    type: OS::Aodh::GnocchiAggregationByResourcesAlarm
    properties:
      description: Scale up if CPU > 80%
      metric: cpu
      aggregation_method: rate:mean
      threshold: 480000000000.0  # Calculated threshold for 80% on 2 vCPUs
      granularity: 300
      evaluation_periods: 2
      resource_type: instance
      comparison_operator: gt
      alarm_actions:
        - str_replace:
            template: trust+url
            params:
              url: {get_attr: [scale_up_policy, signal_url]}
      query:
        str_replace:
          template: '{"=": {"server_group": "stack_id"}}'
          params:
            stack_id: {get_param: "OS::stack_id"}
```

The full Heat template is available at <https://github.com/thobiast/openstack-heat-autoscale-samples>.

You can filter instances by "metering" metadata added by the Heat template using:

- **Retrieve the `rate:mean` values for all instances in a specific server_group:**

```bash
$ openstack metric aggregates --resource-type instance --sort-column timestamp \
'(metric cpu rate:mean)' server_group=<stack_id>
```

- **Calculate the CPU percentage usage for each instance in a server_group:**

```bash
$ openstack metric aggregates --resource-type instance --sort-column timestamp \
'(/ (* (/ (/ (metric cpu rate:mean) 1000000000) <granularity>) 100) <Number_of_vCPUs>)' \
server_group=<stack_id>
```

- **Calculates the average CPU utilization percentage for instances within the server group:**

```bash
$ openstack metric aggregates --resource-type instance --sort-column timestamp \
'(aggregate mean (/ (* (/ (/ (metric cpu rate:mean) 1000000000) <granularity>) 100) <Number_of_vCPUs>)))' \
server_group=<stack_id>
```

>> **Note:** The most recent entry may not be accurate if metrics for all instances are not yet available in Gnocchi.

References:

- <https://docs.openstack.org/heat/latest/>
- <https://docs.openstack.org/ceilometer/latest/admin/telemetry-measurements.html>
- <https://docs.openstack.org/releasenotes/ceilometer/rocky.html>
- <https://github.com/openstack/heat-templates>
- <https://docs.redhat.com/en/documentation/red_hat_openstack_platform/17.1/html-single/auto-scaling_for_instances/index>
- <https://github.com/thobiast/openstack-heat-autoscale-samples>

