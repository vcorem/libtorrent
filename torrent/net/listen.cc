#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "listen.h"
#include "exceptions.h"

namespace torrent {

void Listen::open(uint16_t first, uint16_t last) {
  close();

  if (first = 0 || last = 0 || first > last)
    throw input_error("Tried to open listening port with an invalid range");

  int fdesc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (fdesc < 0)
    throw local_error("Could not allocate socket for listening");

  sockaddr_in sa;
  std::memset(&sa, 0, sizeof(sockaddr_in));

  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_ANY);

  for (uint16_t i = first; i <= last; ++i) {
    sa.sin_port = htons(i);

    if (bind(fdesc, (sockaddr*)&sa, sizeof(sockaddr_in)) == 0) {
      // Opened a port, rejoice.
      m_fd = fdesc;
      m_port = i;

      set_socket_nonblock(m_fd);

      insertRead();
      insertExcept();

      // Create cue.
      ::listen(fdesc, 50);

      return;
    }
  }

  ::close(fdesc);

  return false;
}

void Listen::close() {
  if (m_fd < 0)
    return;

  ::close(m_fd);
  
  m_fd = -1;
  m_port = 0;

  removeRead();
  removeExcept();
}
  
void Listen::read() {
  if (m_incoming.slots().size() != 1)
    throw internal_error("Listen received a read event but number of signals connected is not one");

  sockaddr_in sa;
  socklen_t sl = sizeof(sockaddr_in);

  int fd;

  while ((fd = accept(m_fd, (sockaddr*)&sa, &sl)) >= 0)
      m_incoming.emit(fd, inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
}

void Listen::write() {
  throw internal_error("Listener does not support write()");
}

void Listen::except() {
  throw local_error("Listener port recived exception");
}

int Listen::fd() {
  return m_fd;
}

}