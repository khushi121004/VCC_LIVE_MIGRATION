# Live Migration of Virtual Machines using KVM & Libvirt

## Project Overview

This project explores live migration strategies for Virtual Machines (VMs) using **KVM** and **libvirt**, aiming to reduce downtime and optimize resource usage.

We implemented:
- Full VM Live Migration
- Pre-Copy Migration using NFS
- Post-Copy Migration
- Post-Copy + Access Bit Scanning (Working Set Optimization)

---

## Tools & Dependencies

- Ubuntu 20.04+
- KVM
- libvirt
- virt-manager
- NFS server
- SSH (passwordless setup)
- `scp` utility
- Custom C program for access bit scanning

---

## Setup Instructions

### 1. Install Required Packages
```bash
sudo apt update
sudo apt install qemu-kvm libvirt-daemon-system libvirt-clients bridge-utils virt-manager nfs-kernel-server nfs-common
```

### 2. Create a VM
Use **virt-manager** to create a VM on the source machine.

---

## SSH Passwordless Setup

On source machine:
```bash
ssh-keygen -t rsa
ssh-copy-id user@<destination-ip>
```

---

## Full VM Live Migration
```bash
virsh migrate --live <vm-name> qemu+ssh://<destination-ip>/system
```
---

## Pre-Copy Migration using NFS

### On NFS Server:
```bash
sudo mkdir -p /lib/libvirt-img/images
sudo chown nobody:nogroup /lib/libvirt-img/images
echo "/lib/libvirt-img/images *(rw,sync,no_subtree_check)" | sudo tee -a /etc/exports
sudo exportfs -a
sudo systemctl restart nfs-kernel-server
```

### On Source & Destination:
```bash
sudo mount <nfs-server-ip>:/lib/libvirt-img/images /lib/libvirt-img/images
```

Edit the VM XML:
```bash
virsh edit <vm-name>
# Update disk source path to /mnt/vmshared/<vm-disk-file>
```

Run migration:
```bash
virsh migrate --live <vm-name> qemu+ssh://<destination-ip>/system
```

---

## Post-Copy Migration

Ensure post-copy is enabled and run:
```bash
virsh migrate --live --postcopy <vm-name> qemu+ssh://<destination-ip>/system
```

---

## Access Bit Scanning (Working Set Optimization)

### Compile C program:
```bash
gcc -o access_scan access_scan.c
sudo ./access_scan <pid-of-vm-process> /tmp/hot_pages 5
```

### Send hot pages and migrate:
```bash
chmod +x access_bit.sh
sudo /access_bit.sh
```

---

## Team Members

- **Avani Rai** – B22CS094  
- **Manya** – B22CS032  
- **Khushi Jitendra Agrawal** – B22CS005  
