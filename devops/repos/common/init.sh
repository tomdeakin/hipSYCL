#!/bin/bash

HIPSYCL_PKG_REPO_STAGE_DIR=${HIPSYCL_PKG_REPO_STAGE_DIR:-./stage}

export HIPSYCL_PKG_ARCH_PKG_DIR=$HIPSYCL_PKG_REPO_STAGE_DIR/new_pkg_arch
export HIPSYCL_PKG_CENTOS_PKG_DIR=$HIPSYCL_PKG_REPO_STAGE_DIR/new_pkg_centos
export HIPSYCL_PKG_UBUNTU_PKG_DIR=$HIPSYCL_PKG_REPO_STAGE_DIR/new_pkg_ubuntu


export HIPSYCL_GPG_KEY=B2B75080
