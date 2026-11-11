#!/bin/bash
# Update TeenAstroEfinder files from this repo copy and restart the service.
set -euo pipefail

EF_USER="${SUDO_USER:-efinder}"
EF_HOME="$(getent passwd "$EF_USER" | cut -d: -f6)"
INSTALL_DIR="${EF_HOME}/TeenAstroEfinder"
REPO_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "Updating ${INSTALL_DIR} from ${REPO_DIR}"
rsync -a --exclude '.git' --exclude '__pycache__' --exclude 'tests' \
  --exclude 'efinder.config' \
  "$REPO_DIR/" "$INSTALL_DIR/"
chown -R "$EF_USER:$EF_USER" "$INSTALL_DIR"

sudo systemctl restart teenastro-efinder.service
echo "teenastro-efinder restarted"
