---
nav_order: 20
---

# OSTree Overview
{: .no_toc }

1. TOC
{:toc}

<!-- SPDX-License-Identifier: (CC-BY-SA-3.0 OR GFDL-1.3-or-later) -->

## Introduction

OSTree is an upgrade system for Linux-based operating systems that
performs atomic upgrades of complete filesystem trees.  It is
not a package system; rather, it is intended to complement them.
A primary model is composing packages on a server, and then
replicating them to clients.

The underlying architecture might be summarized as "git for
operating system binaries".  It operates in userspace, and will
work on top of any Linux filesystem.  At its core is a git-like
content-addressed object store with branches (or "refs") to track
meaningful filesystem trees within the store. Similarly, one can
check out or commit to these branches.

Layered on top of that is bootloader configuration, management of
`/etc`, and other functions to perform an upgrade beyond just
replicating files.

You can use OSTree standalone in the pure replication model,
but another approach is to add a package manager on top,
thus creating a hybrid tree/package system.

## Hello World example

OSTree is mostly used as a library, but a quick tour of using its
CLI tools can give a general idea of how it works at its most
basic level.

You can create a new OSTree repository using `init`:

```
$ ostree --repo=repo init
```

This will create a new `repo` directory containing your
repository. Feel free to inspect it.

Now, let's prepare some data to add to the repo:

```
$ mkdir tree
$ echo "Hello world!" > tree/hello.txt
```

We can now import our `tree/` directory using the `commit`
command:

```
$ ostree --repo=repo commit --branch=foo tree/
```

This will create a new branch `foo` pointing to the full tree
imported from `tree/`. In fact, we could now delete `tree/` if we
wanted to.

To check that we indeed now have a `foo` branch, you can use the
`refs` command:

```
$ ostree --repo=repo refs
foo
```

We can also inspect the filesystem tree using the `ls` and `cat`
commands:

```
$ ostree --repo=repo ls foo
d00775 1000 1000      0 /
-00664 1000 1000     13 /hello.txt
$ ostree --repo=repo cat foo /hello.txt
Hello world!
```

And finally, we can check out our tree from the repository:

```
$ ostree --repo=repo checkout foo tree-checkout/
$ cat tree-checkout/hello.txt
Hello world!
```

## Comparison with "package managers"

Because OSTree is designed for deploying core operating
systems, a comparison with traditional "package managers" such
as dpkg and rpm is illustrative.  Packages are traditionally
composed of partial filesystem trees with metadata and scripts
attached, and these are dynamically assembled on the client
machine, after a process of dependency resolution.

In contrast, OSTree only supports recording and deploying
*complete* (bootable) filesystem trees.  It
has no built-in knowledge of how a given filesystem tree was
generated or the origin of individual files, or dependencies,
descriptions of individual components.  Put another way, OSTree
only handles delivery and deployment; you will likely still want
to include inside each tree metadata about the individual
components that went into the tree.  For example, a system
administrator may want to know what version of OpenSSL was
included in your tree, so you should support the equivalent of
`rpm -q` or `dpkg -L`.

The OSTree core emphasizes replicating read-only OS trees via
HTTP, and where the OS includes (if desired) an entirely
separate mechanism to install applications, stored in `/var` if they're system global, or
`/home` for per-user
application installation.  An example application mechanism is
<http://docker.io/>

However, it is entirely possible to use OSTree underneath a
package system, where the contents of `/usr` are computed on the client.
For example, when installing a package, rather than changing the
currently running filesystem, the package manager could assemble
a new filesystem tree that layers the new packages on top of a
base tree, record it in the local OSTree repository, and then
set it up for the next boot.  To support this model, OSTree
provides an (introspectable) C shared library.

## Comparison with block/image replication

OSTree shares some similarity with "dumb" replication and
stateless deployments, such as the model common in "cloud"
deployments where nodes are booted from an (effectively)
readonly disk, and user data is kept on a different volumes.
The advantage of "dumb" replication, shared by both OSTree and
the cloud model, is that it's *reliable*
and *predictable*.

But unlike many default image-based deployments, OSTree supports
exactly two persistent writable directories that are preserved across
upgrades: `/etc` and `/var`.

Because OSTree operates at the Unix filesystem layer, it works
on top of any filesystem or block storage layout; it's possible
to replicate a given filesystem tree from an OSTree repository
into plain ext4, BTRFS, XFS, or in general any Unix-compatible
filesystem that supports hard links.  Note: OSTree will
transparently take advantage of some BTRFS features if deployed
on it.

OSTree is orthogonal to virtualization mechanisms like AMIs and qcow2
images. It targets updating stateful VMs in-place, rather than generating
new images.

In practice, users of "bare metal" configurations will find the OSTree
model most useful.

## Atomic transitions between parallel-installable read-only filesystem trees

Another deeply fundamental difference between both package
managers and image-based replication is that OSTree is
designed to parallel-install *multiple versions* of multiple
*independent* operating systems.  OSTree
relies on a new toplevel `ostree` directory; it can in fact
parallel install inside an existing OS or distribution
occupying the physical `/` root.

On each client machine, there is an OSTree repository stored
in `/ostree/repo`, and a set of "deployments" stored in `/ostree/deploy/$STATEROOT/$CHECKSUM`.
Each deployment is primarily composed of a set of hardlinks
into the repository.  This means each version is deduplicated;
an upgrade process only costs disk space proportional to the
new files, plus some constant overhead.

The model OSTree emphasizes is that the OS read-only content
is kept in the classic Unix `/usr`; it comes with code to
create a Linux read-only bind mount to prevent inadvertent
corruption.  There is exactly one `/var` writable directory shared
between each deployment for a given OS.  The OSTree core code
does not touch content in this directory; it is up to the code
in each operating system for how to manage and upgrade state.

Finally, each deployment has its own writable copy of the
configuration store `/etc`.  On upgrade, OSTree will
perform a basic 3-way diff, and apply any local changes to the
new copy, while leaving the old untouched.
