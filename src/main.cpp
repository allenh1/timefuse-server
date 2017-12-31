/* Qt Includes */
#include <QtCore>
#include <QCoreApplication>

/* STL Includes */
#include <iostream>

/* File Includes */
#include "src/master_node.hpp"
#include "src/worker_node.hpp"

int main(int argc, char ** argv)
{
  QCoreApplication app(argc, argv);
  QStringList args = app.arguments();

  if (argc < 2) {goto error;} else if (!strcmp(argv[1], "--worker")) {
    /* check the arg list for specific host */
    QString master_host = "localhost";
    quint16 master_port = 3224;
    QString worker_host = "localhost";
    quint16 worker_port = 3442;

    if (args.filter("--mhost").size()) {
      master_host = args.filter("--mhost")[0];
      /* trim the hostname parameter */
      master_host.replace("--mhost=", "");
    }
    if (args.filter("--whost").size()) {
      worker_host = args.filter("--whost")[0];
      /* trim the hostname parameter */
      worker_host.replace("--whost=", "");
    }
    if (args.filter("--mport").size()) {
      QString port = args.filter("--mport")[0];
      port.replace("--mport=", ""); bool ok;
      master_port = port.toUShort(&ok);
      if (!ok) {goto error;}
    }
    if (args.filter("--wport").size()) {
      QString port = args.filter("--wport")[0];
      port.replace("--wport=", ""); bool ok;
      worker_port = port.toUShort(&ok);
      if (!ok) {goto error;}
    }

    worker_node worker(worker_host, worker_port);
    worker.set_master_hostname(master_host);
    worker.set_master_port(master_port);
    worker.init();
    return app.exec();
  } else if (!strcmp(argv[1], "--master")) {
    QString master_host = "localhost";
    quint16 master_port = 3224;

    if (args.filter("--mhost").size()) {
      master_host = args.filter("--mhost")[0];
      /* trim the hostname parameter */
      master_host.replace("--mhost=", "");
    }
    if (args.filter("--mport").size()) {
      QString port = args.filter("--mport")[0];
      port.replace("--mport=", ""); bool ok;
      master_port = port.toUShort(&ok);
      if (!ok) {goto error;}
    }

    master_node master(master_host, master_port);
    master.init();
    return app.exec();
  } else {goto error;}

error:
  std::cerr << "Error: invalid arguments!" << std::endl;
  std::cerr << "Usage:" << std::endl;
  std::cerr << "\t" << argv[0] << " [--worker]|[--master]" << std::endl;
  std::cerr << "Optional: " << std::endl;
  std::cerr << "\t[--mhost=<master>]" << std::endl;
  std::cerr << "\t[--whost=<worker>]" << std::endl;
  std::cerr << "\t[--mport=<master port>]" << std::endl;
  std::cerr << "\t[--wport=<worker port]" << std::endl;
  return 1;
}
