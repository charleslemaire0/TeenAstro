#pragma once

#ifdef EMU_MAINUNIT

/** Background HTTP GET /cmd?q=… proxy → raw LX200 TCP (emu USB serial port). */
void emu_http_cmd_proxy_start(int http_listen_port, int lx200_tcp_port);

#endif
