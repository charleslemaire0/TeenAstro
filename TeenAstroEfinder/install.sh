#!/bin/bash
# Install TeenAstroEfinder on a Raspberry Pi Zero 2W (Bookworm Lite 64-bit).
set -euo pipefail

EF_USER="${SUDO_USER:-efinder}"
EF_HOME="$(getent passwd "$EF_USER" | cut -d: -f6)"
INSTALL_DIR="${EF_HOME}/TeenAstroEfinder"
REPO_DIR="$(cd "$(dirname "$0")" && pwd)"
VENV="${EF_HOME}/venv-efinder"

echo "*****************************************************************************"
echo "TeenAstroEfinder install"
echo "*****************************************************************************"

sudo apt update
sudo apt install -y \
  python3-pip python3-serial python3-psutil python3-pil python3-skyfield \
  python3-picamera2 git gpsd

# UART: enable serial, disable console
if command -v raspi-config >/dev/null 2>&1; then
  sudo raspi-config nonint do_serial_hw 0 || true
  sudo raspi-config nonint do_serial_cons 1 || true
fi
# Bookworm: ensure UART overlay
if [ -f /boot/firmware/config.txt ]; then
  CFG=/boot/firmware/config.txt
elif [ -f /boot/config.txt ]; then
  CFG=/boot/config.txt
else
  CFG=""
fi
if [ -n "$CFG" ]; then
  sudo grep -q '^enable_uart=1' "$CFG" || echo 'enable_uart=1' | sudo tee -a "$CFG" >/dev/null
  sudo sed -i 's/^dtoverlay=disable-bt/#dtoverlay=disable-bt/' "$CFG" || true
fi

echo "*****************************************************************************"
echo "Copying application to ${INSTALL_DIR}"
echo "*****************************************************************************"
sudo -u "$EF_USER" mkdir -p "$INSTALL_DIR/images" "$INSTALL_DIR/data" /var/tmp/solve
rsync -a --exclude '.git' --exclude '__pycache__' --exclude 'tests' \
  "$REPO_DIR/" "$INSTALL_DIR/"
chown -R "$EF_USER:$EF_USER" "$INSTALL_DIR"

echo "*****************************************************************************"
echo "Python venv"
echo "*****************************************************************************"
if [ ! -d "$VENV" ]; then
  sudo -u "$EF_USER" python3 -m venv "$VENV" --system-site-packages
fi
"$VENV/bin/pip" install --upgrade pip
"$VENV/bin/pip" install grpcio grpcio-tools pyserial pillow

# Optional ASI SDK (from upstream eFinder Lite release assets if present in repo)
if [ -f "$REPO_DIR/ASI_linux_mac_SDK_V1.31.tar.bz2" ]; then
  echo "Installing ASI SDK"
  tmp=$(mktemp -d)
  tar xf "$REPO_DIR/ASI_linux_mac_SDK_V1.31.tar.bz2" -C "$tmp"
  sudo mkdir -p /lib/zwoasi/armv8
  sudo cp "$tmp"/ASI_linux_mac_SDK_*/lib/armv8/*.* /lib/zwoasi/armv8/ || true
  sudo install "$tmp"/ASI_linux_mac_SDK_*/lib/asi.rules /lib/udev/rules.d/ || true
  "$VENV/bin/pip" install zwoasi
  rm -rf "$tmp"
fi

# Large tetra3 databases / de421 — download from upstream if missing
DATA="$INSTALL_DIR/data"
sudo -u "$EF_USER" mkdir -p "$DATA"
if [ ! -f "$EF_HOME/de421.bsp" ]; then
  echo "Fetching de421.bsp (skyfield will also fetch on first use if missing)"
fi

echo "tmpfs /var/tmp tmpfs nodev,nosuid,size=32M 0 0" | sudo tee /etc/fstab.d/teenastro-efinder >/dev/null 2>&1 || \
  (grep -q 'tmpfs /var/tmp' /etc/fstab || echo "tmpfs /var/tmp tmpfs nodev,nosuid,size=32M 0 0" | sudo tee -a /etc/fstab >/dev/null)

echo "*****************************************************************************"
echo "systemd service"
echo "*****************************************************************************"
sudo tee /etc/systemd/system/teenastro-efinder.service >/dev/null <<EOF
[Unit]
Description=TeenAstro eFinder plate-solver
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=${EF_USER}
WorkingDirectory=${INSTALL_DIR}
Environment=EFINDER_CONFIG=${INSTALL_DIR}/efinder.config
ExecStart=${VENV}/bin/python ${INSTALL_DIR}/efinder.py
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable teenastro-efinder.service

echo "*****************************************************************************"
echo "Done. Reboot recommended so UART settings apply."
echo "  sudo systemctl start teenastro-efinder"
echo "*****************************************************************************"
