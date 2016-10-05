/* Qt Includes */
#include <QtCore>
#include <QCoreApplication>

/* STL Includes */
#include <iostream>

/* File Includes */
#include "src/master.hpp"
/* @todo implement "src/slave.hpp" */

int main (int argc, char ** argv)
{
   QCoreApplication app(argc, argv);

   if (argc < 2) goto error;
   else if (!strcmp(argv[1], "--slave")) {
	  /* @todo slave mode */
	  /* slave_node slave(argc, argv); */
	  /* slave.init(); */
	  return app.exec();
   } else if (!strcmp(argv[1], "--master")) {
	  /* master mode */
	  /* @todo set args from argv */
	  QString hostname = "localhost";
	  quint16 port = 3224;
	  
	  master_node master(hostname, port);
	  master.init();
	  return app.exec();
   } else goto error;

error:
   std::cerr<<"Error: invalid arguments!"<<std::endl;
   std::cerr<<"Usage: "<<argv[0]<<" [--slave]|[--master]"<<std::endl;
   return 1;
}
