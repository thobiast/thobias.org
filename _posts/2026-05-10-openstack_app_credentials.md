---
layout: post
title:  "Stop using user passwords for OpenStack automation"
date:   2026-05-10
---

<a href="https://www.openstack.org/">
  <img src="/assets/images/openstack.png" alt="OpenStack site" title="OpenStack" width="110">
</a>


* This line is needed, but won't appear.
{:toc}


## 1. Introduction

As an OpenStack operator, administrator, or user, automation eventually becomes part of the work.
It may be a small script, a CI/CD pipeline, a Terraform job, an Ansible playbook, a monitoring check, a backup process, a third-party tool, or an application using the OpenStack SDK.
Eventually, that automation needs to authenticate to OpenStack without human interaction.

The easy path is usually to use a user password.
But storing a user's OpenStack password in environment variables, configuration files, CI/CD secrets, container images, or tool configuration is risky.

It gets worse when users authenticate through LDAP, Active Directory, Single Sign-On  (SSO), or another central identity provider.
In that case, the password may not be "just the OpenStack password". It may also provide access to other systems.
If that password leaks, the impact can be much bigger than the original automation use case.

The [OpenStack Application Credentials](https://docs.openstack.org/keystone/latest/user/application_credentials.html)
feature was introduced in the [OpenStack Queens release](https://docs.openstack.org/releasenotes/keystone/queens.html#relnotes-13-0-0-stable-queens).
It provides a safer option for applications and automation tools by letting them authenticate with a dedicated credential ID and secret instead of a user password.

#### Why use Application Credentials

You do not need to reuse the same user password everywhere.

You can create separate credentials for different jobs or tools: one for monitoring, one for Terraform runs, one for an inventory script, one for backups, and so on.

Each Application Credential can be configured for a specific purpose.
When creating it, you can explicitly choose the [Keystone role(s)](https://docs.openstack.org/keystone/latest/admin/service-api-protection.html),
set an expiration date, and add [Access Rules](https://docs.openstack.org/keystone/latest/user/application_credentials.html#access-rules).
This makes the principle of least privilege easier to apply: give the application only the access it actually needs.

The monitoring credential does not need the same access as Terraform, and the backup credential does not need the same access as a deployment pipeline.

Application Credentials are also project-scoped. They are created for the project where your current token is scoped.

The secret is separate from the user's password.
If one credential leaks or the automation is no longer needed, you can delete or rotate that credential without changing the user's password or affecting unrelated tools.

In other words: do not give automation your personal password if you can give it a smaller, safer credential instead.

## 2. Choosing the right role first

Before creating an Application Credential, it is a good idea to check which roles your user has in the current project.

To see the roles assigned to your authenticated user in the current project, run:

```bash
$ openstack role assignment list --auth-user --auth-project --names
```

If your cloud uses groups, inherited roles, LDAP, federation, or SSO mappings, it is also useful to check the effective role assignments:
The `--effective` option returns only effective role assignments, which is useful when the role may come from group membership or inheritance.

```bash
$ openstack role assignment list --auth-user --auth-project --effective --names
+------------+--------+-------+---------------+--------+--------+-----------+
| Role       | User   | Group | Project       | Domain | System | Inherited |
+------------+--------+-------+---------------+--------+--------+-----------+
| member     | myuser |       | demo-project  |        |        | False     |
| admin      | myuser |       | demo-project  |        |        | False     |
| reader     | myuser |       | demo-project  |        |        | False     |
+------------+--------+-------+---------------+--------+--------+-----------+
```

Keystone creates the Application Credential for the project to which your current token is scoped.
If you do not specify the role, Keystone assigns the Application Credential the same roles that are present in your current token.
This does not mean every role the user has in every project or domain. It means the roles from the current project-scoped token.
For example, if the user is currently scoped to a project where they have `admin`, `member`, and `reader`, the Application Credential receives all of those roles.

```bash
$ openstack application credential create monitoring-default-roles
+--------------+--------------------------------------------------+
| Field        | Value                                            |
+--------------+--------------------------------------------------+
| description  | None                                             |
| expires_at   | None                                             |
| id           | 26bb287fd56a41f8a577c47f79221187                 |
| name         | monitoring-default-roles                         |
| project_id   | e99b6f4b9bf84a9da27e20c9cbfe887a                 |
| roles        | admin member reader                              |
| secret       | <secret shown once at creation time>             |
| unrestricted | False                                            |
+--------------+--------------------------------------------------+
```

This usually violates the least privilege principle by granting excessive permissions.
A safer approach is to explicitly specify the lowest role the application needs:

```bash
$ openstack application credential create monitoring-reader \
  --role reader
+--------------+--------------------------------------------------+
| Field        | Value                                            |
+--------------+--------------------------------------------------+
| description  | None                                             |
| expires_at   | None                                             |
| id           | c0a1371f9c824b2891a7deb0223f3ed0                 |
| name         | monitoring-reader                                |
| project_id   | e99b6f4b9bf84a9da27e20c9cbfe887a                 |
| roles        | reader                                           |
| secret       | <secret shown once at creation time>             |
| unrestricted | False                                            |
+--------------+--------------------------------------------------+
```

If the application really requires more than one role, you can specify multiple roles by repeating the `--role` option:

```bash
$ openstack application credential create monitoring-reader-member \
  --role reader \
  --role member
```

This makes the credential's purpose clearer and reduces the impact if the credential is leaked.


## 3. Creating a basic Application Credential

[OpenStack CLI](https://docs.openstack.org/python-openstackclient/latest/cli/)
has the following commands to interact with Application Credentials:

```bash
$ openstack application credential --help
Command "application" matches:
  application credential create
  application credential delete
  application credential list
  application credential show
```

After choosing the right role, create the Application Credential.

```bash
$ openstack application credential create monitoring-reader-basic \
    --description "Read-only credential for monitoring script" \
    --role reader
+--------------+--------------------------------------------------+
| Field        | Value                                            |
+--------------+--------------------------------------------------+
| description  | Read-only credential for monitoring script       |
| expires_at   | None                                             |
| id           | 54c96fc230984fa89c00099fafd8c21b                 |
| name         | monitoring-reader-basic                          |
| project_id   | e99b6f4b9bf84a9da27e20c9cbfe887a                 |
| roles        | reader                                           |
| secret       | <secret shown once at creation time>             |
| system       | None                                             |
| unrestricted | False                                            |
+--------------+--------------------------------------------------+
```

Save the id and secret securely. **The secret is only shown when the Application Credential is created**.
Keystone does not show it again later, so if the secret is lost, you need to create a new Application Credential.

For temporary or automation-specific credentials, it is a good idea to set an expiration date. The
[--expiration](https://docs.openstack.org/python-openstackclient/latest/cli/command-objects/application-credentials.html)
option uses the format `YYYY-mm-ddTHH:MM:SS`. If no expiration is provided, the Application Credential does not expire.

On Linux systems using GNU date, you can generate an expiration timestamp seven days from now:

```bash
$ openstack application credential create monitoring-reader-7d \
    --description "Read-only credential for monitoring script" \
    --role reader \
    --expiration "$(date -u -d '+7 days' '+%Y-%m-%dT%H:%M:%S')"
+--------------+--------------------------------------------------+
| Field        | Value                                            |
+--------------+--------------------------------------------------+
| description  | Read-only credential for monitoring script       |
| expires_at   | 2026-05-13T20:38:16.000000                       |
| id           | 486dd8efdd524a8a96ef4427611f76c0                 |
| name         | monitoring-reader-7d                             |
| project_id   | e99b6f4b9bf84a9da27e20c9cbfe887a                 |
| roles        | reader                                           |
| secret       | <secret shown once at creation time>             |
| system       | None                                             |
| unrestricted | False                                            |
+--------------+--------------------------------------------------+
```

After creating the credential, you can confirm its details:

```bash
$ openstack application credential show monitoring-reader-7d
+--------------+--------------------------------------------------+
| Field        | Value                                            |
+--------------+--------------------------------------------------+
| description  | Read-only credential for monitoring script       |
| expires_at   | 2026-05-13T20:38:16.000000                       |
| id           | 486dd8efdd524a8a96ef4427611f76c0                 |
| name         | monitoring-reader-7d                             |
| project_id   | e99b6f4b9bf84a9da27e20c9cbfe887a                 |
| roles        | reader                                           |
| system       | None                                             |
| unrestricted | False                                            |
+--------------+--------------------------------------------------+
```

Notice that the secret is not shown in the show output.

You can also list all Application Credentials owned by your user:

```bash
$ openstack application credential list
+----------------------------------+----------------------+----------------------------------+--------------------------------------------+----------------------------+
| ID                               | Name                 | Project ID                       | Description                                | Expires At                 |
+----------------------------------+----------------------+----------------------------------+--------------------------------------------+----------------------------+
| 54c96fc230984fa89c00099fafd8c21b | monitoring-reader    | e99b6f4b9bf84a9da27e20c9cbfe887a | Read-only credential for monitoring script | None                       |
| 486dd8efdd524a8a96ef4427611f76c0 | monitoring-reader-7d | e99b6f4b9bf84a9da27e20c9cbfe887a | Read-only credential for monitoring script | 2026-05-13T20:38:16.000000 |
+----------------------------------+----------------------+----------------------------------+--------------------------------------------+----------------------------+
```

**One important detail is that Application Credentials should be treated as immutable.**

After creating an Application Credential, you cannot simply edit it later to change the role, expiration date, secret, description, or access rules.
OpenStackClient provides commands to create, list, show, and delete Application Credentials, but not an update or set command.

If you need to change something, create a new Application Credential, update the script or application to use the new ID and secret, and then delete the old one.

### Restricted vs unrestricted Application Credentials

By default, Application Credentials are **restricted**. In the command output, this appears as:

```console
| unrestricted | False |
```

A restricted Application Credential can still use the roles and Access Rules assigned to it,
but it cannot create or delete other Application Credentials or [trusts](https://docs.openstack.org/keystone/latest/user/trusts.html).

This is an important security protection. If an Application Credential is leaked, we do not want it to create another credential and keep access after the original credential is deleted.

The default behavior is equivalent to using `--restricted`:

```bash
$ openstack application credential create monitoring-reader \
    --description "Read-only credential for monitoring script" \
    --role reader \
    --restricted
```

To create an unrestricted Application Credential:

```bash
$ openstack application credential create credential-manager \
    --description "Credential allowed to manage application credentials" \
    --role member \
    --unrestricted
```

Use `--unrestricted` only when the application really needs to create or delete Application Credentials or trusts.

Also, `--unrestricted` does not grant extra OpenStack permissions by itself.
The credential still needs the required role and must be allowed by Keystone policy.
It only removes the special protection that prevents Application Credentials from managing other Application Credentials and trusts.

## 4. Using the Application Credential

To use an Application Credential, you should authenticate using the `v3applicationcredential` as the [authentication type](https://docs.openstack.org/python-openstackclient/latest/cli/authentication.html).

> auth_type: v3applicationcredential

**Important:** When using the `v3applicationcredential` authentication type, you cannot specify project name or ID, like `OS_PROJECT_ID`, `OS_PROJECT_NAME`, `OS_TENANT_ID` or `OS_TENANT_NAME`.

Application Credentials already contain their scope.
The Keystone API says that authentication with an Application Credential cannot request a scope because the scope information is retrieved from the Application Credential.

### 4.1 Using clouds.yaml file

Create or update [clouds.yaml configuration file](https://docs.openstack.org/python-openstackclient/latest/configuration/index.html#configuration-files)
in the current directory or in `~/.config/openstack`:

```yaml
clouds:
  demo_appcred:
    auth:
      auth_url: https://your_keystone.openstack:5000/v3
      application_credential_id: <app_cred_id>
      application_credential_secret: <app_cred_secret>
    auth_type: v3applicationcredential
    # optional parameters
    region_name: RegionOne
    interface: public
    identity_api_version: 3
```

Then, use the `--os-cloud` option:

```bash
$ openstack --os-cloud demo_appcred server list
```

### 4.2 Using environment variables

```bash
$ export OS_AUTH_URL="https://your_keystone.openstack:5000/v3"
$ export OS_AUTH_TYPE=v3applicationcredential
$ export OS_APPLICATION_CREDENTIAL_ID=<app_cred_id>
$ export OS_APPLICATION_CREDENTIAL_SECRET=<app_cred_secret>
$ # optional parameters
$ export OS_REGION_NAME="RegionOne"
$ export OS_IDENTITY_API_VERSION=3
$ export OS_INTERFACE=public
$ openstack server list
```

For example, this is the error you may see when `OS_PROJECT_ID` is set:

```bash
$ export OS_PROJECT_ID=78db65854be04d6983749dd6d0d26962
$ openstack server list
Error authenticating with application credential: Application credentials cannot request a scope. (HTTP 401) (Request-ID: req-c1824004-65df-4cdb-bf92-dbfc16c24b46)
```

### 4.3 Using OpenStack Python SDK

```python
import openstack

conn = openstack.connect(
    auth_url="https://your_keystone.openstack:5000/v3",
    auth_type="v3applicationcredential",
    application_credential_id="<app-cred-id>",
    application_credential_secret="<app_cred_secret>",
)

for server in conn.compute.servers():
    print(server.name)
```

## 5. Access rules

Roles are the first level of restriction. [Access Rules](https://docs.openstack.org/keystone/latest/user/application_credentials.html#access-rules)
add another layer by limiting which API requests the Application Credential can make.

An Access Rule has three fields:

```json
{
  "service": "<service_type>",
  "method": "<HTTP method>",
  "path": "<request path>"
}
```

- `service` is the OpenStack service type, such as `compute`, `network`, `image`, `volumev3`, or `load-balancer`.
- `method` is the HTTP method, such as `GET`, `POST`, `PUT`, `PATCH`, or `DELETE`.
- `path` is the API path used by the OpenStack service.

Access Rules only restrict permissions. They do not grant extra permissions. The Application Credential still needs a role that allows the operation.

### 5.1 Checking available service types

To see the `service types` available in your cloud, run:

```bash
$ openstack service list -c Name -c Type
+-----------+----------------+
| Name      | Type           |
+-----------+----------------+
| glance    | image          |
| keystone  | identity       |
| neutron   | network        |
| nova      | compute        |
| placement | placement      |
| cinderv3  | volumev3       |
| octavia   | load-balancer  |
| heat      | orchestration  |
| barbican  | key-manager    |
+-----------+----------------+
```

In the Access Rule, use the value from the "Type" column.

**Operator note: service_type must be configured**

For Access Rules to work, the OpenStack service using `keystonemiddleware` must know its service type. This is configured in the service configuration file under `[keystone_authtoken]`.

For example, Cinder v3 would use:

```ini
[keystone_authtoken]
service_type = volumev3
```

Nova would normally use:

```ini
[keystone_authtoken]
service_type = compute
```

Neutron would normally use:

```ini
[keystone_authtoken]
service_type = network
```

Glance would normally use:

```ini
[keystone_authtoken]
service_type = image
```

### 5.2 Find the API method and path in the OpenStack API reference

The `method` and `path` come from the API documentation of each OpenStack service. You can check the official documentation. Some examples:

- <https://docs.openstack.org/api-ref/image/>
- <https://docs.openstack.org/api-ref/compute/>
- <https://docs.openstack.org/api-ref/network/v2/>
- <https://docs.openstack.org/api-ref/block-storage/v3/>
- <https://docs.openstack.org/api-ref/load-balancer/v2/>

The API documentation is the best place to start, but you can also confirm the exact method and path using the `--debug` of the CLI:

```bash
$ openstack --debug server list 2>&1 | grep -E '^REQ:'
```

### 5.3 Basic example

Create an Access Rule that only allows the Application Credential to list Glance images:

```bash
$ cat > image_listonly.json <<'EOF'
[
  {
    "service": "image",
    "method": "GET",
    "path": "/v2/images"
  }
]
EOF
$ openstack application credential create image-list \
  --role reader \
  --access-rules ./image_listonly.json
+--------------+---------------------------------------------------------------------------------------------------------+
| Field        | Value                                                                                                   |
+--------------+---------------------------------------------------------------------------------------------------------+
| access_rules | [{'id': 'deed7b9b59c14293b35d35ddca0133c7', 'service': 'image', 'path': '/v2/images', 'method': 'GET'}] |
| description  | None                                                                                                    |
| expires_at   | None                                                                                                    |
| id           | d67f1b26b5324214bb4cc53fb7777cdb                                                                        |
| name         | image-list                                                                                              |
| project_id   | 20c4202291d64c2ca48e6144840dc1a5                                                                        |
| roles        | reader                                                                                                  |
| secret       | <secret shown once at creation time>                                                                    |
| system       | None                                                                                                    |
| unrestricted | False                                                                                                   |
+--------------+---------------------------------------------------------------------------------------------------------+
```

```bash
$ # Configure clouds.yaml or env var to use this new application credential, then test it
$ openstack --os-cloud demo_appcred image list
+--------------------------------------+------------------+--------+
| ID                                   | Name             | Status |
+--------------------------------------+------------------+--------+
| 7a9b1cf0-3d4b-4b87-80ce-380f74d6324c | cirros           | active |
| 13b9201e-c0cf-48fb-83c3-e35d7e115af4 | ubuntu-noble     | active |
+--------------------------------------+------------------+--------+
$  openstack --os-cloud demo_appcred image show 13b9201e-c0cf-48fb-83c3-e35d7e115af4
HttpException: 401: Client Error for url: https://your_glance.openstack:9292/v2/images/13b9201e-c0cf-48fb-83c3-e35d7e115af4, 401 Unauthorized: This server could not verify that you are authorized to access the document you requested. Either you supplied the wrong credentials (e.g., bad password), or your browser does not understand how to supply the credentials required.
$ openstack --os-cloud demo_appcred server list
HttpException: 401: Client Error for url: https://nova.openstack:8774/v2.1/servers/detail?deleted=False, The request you have made requires authentication.
```

This Access Rule only allows listing images. It denies the image show request.
We can check the [Glance API reference](https://docs.openstack.org/api-ref/image/v2/index.html#show-image):

- `GET - /v2/images` - List images
- `GET - /v2/images/{image_id}` - Show image

In the access rule created, it only allowed access to `"/v2/images"`.

**Listing and reusing Access Rules**

Access Rules are created during the Application Credential creation process.
There is no standalone `openstack access rule create` command.
After they exist, they can be listed, shown, reused, and deleted.

List existing Access Rules:

```bash
$ openstack access rule list
+----------------------------------+---------+--------+------------+
| ID                               | Service | Method | Path       |
+----------------------------------+---------+--------+------------+
| deed7b9b59c14293b35d35ddca0133c7 | image   | GET    | /v2/images |
+----------------------------------+---------+--------+------------+
```

An Access Rule created for one Application Credential can be reused by another Application Credential by passing its ID with `--access-rules '[{"id": "xxxxxx"}]'`

```bash
$ openstack application credential create image-list-2 \
  --role reader \
  --access-rules '[{"id": "deed7b9b59c14293b35d35ddca0133c7"}]'
+--------------+---------------------------------------------------------------------------------------------------------+
| Field        | Value                                                                                                   |
+--------------+---------------------------------------------------------------------------------------------------------+
| access_rules | [{'id': 'deed7b9b59c14293b35d35ddca0133c7', 'service': 'image', 'path': '/v2/images', 'method': 'GET'}] |
| description  | None                                                                                                    |
| expires_at   | None                                                                                                    |
| id           | cdc1a7d126b5470b8397411f810a400b                                                                        |
| name         | image-list-2                                                                                            |
| project_id   | 20c4202291d64c2ca48e6144840dc1a5                                                                        |
| roles        | reader                                                                                                  |
| secret       | <secret shown once at creation time>                                                                    |
| system       | None                                                                                                    |
| unrestricted | False                                                                                                   |
+--------------+---------------------------------------------------------------------------------------------------------+
```

Now both application credentials point to the same access rule:

```bash
$ openstack application credential show image-list -c access_rules
+--------------+---------------------------------------------------------------------------------------------------------+
| Field        | Value                                                                                                   |
+--------------+---------------------------------------------------------------------------------------------------------+
| access_rules | [{'id': 'deed7b9b59c14293b35d35ddca0133c7', 'service': 'image', 'path': '/v2/images', 'method': 'GET'}] |
+--------------+---------------------------------------------------------------------------------------------------------+
$ openstack application credential show image-list-2 -c access_rules
+--------------+---------------------------------------------------------------------------------------------------------+
| Field        | Value                                                                                                   |
+--------------+---------------------------------------------------------------------------------------------------------+
| access_rules | [{'id': 'deed7b9b59c14293b35d35ddca0133c7', 'service': 'image', 'path': '/v2/images', 'method': 'GET'}] |
+--------------+---------------------------------------------------------------------------------------------------------+
```

**Important:** Even if you delete both application credentials ("image-list" and "image-list-2"), the access rule will not be deleted.
Access Rules are separate objects. If an Access Rule is still used by an Application Credential, Keystone will not allow it to be deleted.
After deleting the Application Credentials that use it, delete the Access Rule manually if it is no longer needed.

```bash
$ openstack application credential delete image-list
$ openstack application credential delete image-list-2
$ openstack access rule list
+----------------------------------+---------+--------+------------+
| ID                               | Service | Method | Path       |
+----------------------------------+---------+--------+------------+
| deed7b9b59c14293b35d35ddca0133c7 | image   | GET    | /v2/images |
+----------------------------------+---------+--------+------------+
$ openstack access rule delete deed7b9b59c14293b35d35ddca0133c7
```

### 5.4 Wildcards

As stated in [official access rules documentation](https://docs.openstack.org/keystone/latest/user/application_credentials.html#access-rules),
The `path` attribute of an Application Credential Access Rule supports wildcard syntax.

We can create an Access rule to allow the image list and image show API calls:

```bash
$ cat > image_list_and_show.json <<'EOF'
[
  {
    "service": "image",
    "method": "GET",
    "path": "/v2/images"
  },
  {
    "service": "image",
    "method": "GET",
    "path": "/v2/images/*"
  }
]
EOF
```

For example, to create an application credential that is restricted to
[listing server IP addresses](https://docs.openstack.org/api-ref/compute/#list-ips), you could use:

```json
[
  {
    "path": "/v2.1/servers/*/ips",
    "method": "GET",
    "service": "compute"
  }
]
```

For even more flexibility, the recursive wildcard `**` indicates that request paths containing any number of `/` characters will be matched. For example:

```json
[
  {
    "path": "/v2.1/**",
    "method": "GET",
    "service": "compute"
 }
]
```

### 5.5 Access rules examples

Access rules to only allow, [list and show of load balancer and listeners](https://docs.openstack.org/api-ref/load-balancer/v2/#list-load-balancers).

```bash
$ cat > lb_and_listener_reader.json <<'EOF'
[
  {
    "service": "load-balancer",
    "method": "GET",
    "path": "/v2.0/lbaas/loadbalancers"
  },
  {
    "service": "load-balancer",
    "method": "GET",
    "path": "/v2.0/lbaas/loadbalancers/{loadbalancer_id}"
  },
  {
    "service": "load-balancer",
    "method": "GET",
    "path": "/v2.0/lbaas/listeners"
  },
  {
    "service": "load-balancer",
    "method": "GET",
    "path": "/v2.0/lbaas/listeners/*"
  }
]
EOF

$ openstack application credential create lb_monitoring \
  --role 83fe4329bf6a4253b0f394ecda8216e6 \
  --access-rules ./lb_and_listener_reader.json
```

Depending on the cloud policy, the `reader` role may not be enough to read load balancers.
In this example, the credential uses a load-balancer-specific role, such as `load-balancer_member`. Use the lowest role available in your cloud that allows the required operation.

This rule allows only `GET` requests to the Nova compute API under /v2.1/:

```bash
$ cat > compute-reader-rules.json <<'EOF'
[
  {
    "service": "compute",
    "method": "GET",
    "path": "/v2.1/**"
  }
]
EOF
```

Access rule to [create servers only](https://docs.openstack.org/api-ref/compute/#create-server):

```json
[
    {
        "path": "/v2.1/servers",
        "method": "POST",
        "service": "compute"
    }
]
```

## 6. User lifecycle consideration

Application Credentials are owned by the user who created them.

If that user is deleted, disabled, or loses the role assignment on the project, the Application Credential will stop working. This is good for security, but it is important for production automation.

For personal scripts, this may be fine. For team or production automation, consider creating the Application Credential from a dedicated automation user, or rotate the credential before disabling the user who created it.

OpenStack's rotation guidance follows this workflow: create a new credential, update the application, then delete the old credential. It also notes that Application Credential names must be unique within the user's credentials.

## 7. Conclusion

OpenStack Application Credentials are a safer way to authenticate scripts, automation tools, monitoring systems, and applications.

They avoid storing a user's OpenStack password and can be limited by project, role, expiration date, and Access Rules.

The main idea is simple: do not put a personal OpenStack password in automation. Create an application credential and grant only the minimum permissions required for the job.

## Useful links

- <https://docs.openstack.org/keystone/latest/user/application_credentials.html>
- <https://docs.openstack.org/python-openstackclient/latest/cli/command-objects/access-rules.html>
- <https://docs.openstack.org/python-openstackclient/latest/cli/authentication.html>
- <https://docs.openstack.org/python-openstackclient/latest/configuration/index.html>
- <https://docs.openstack.org/openstacksdk/latest/user/config/configuration.html>
- <https://docs.openstack.org/keystone/latest/admin/service-api-protection.html>
- <https://docs.openstack.org/api-ref/>
