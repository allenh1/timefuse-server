#ifndef __SLAVE_HPP__
#define __SLAVE_HPP__
/**
 * This class has all attributes relating to a
 * slave node. This is the big task.
 */
class workernode : public QObject
{
  Q_OBJECT
   /**
	* @todo Add required funcitons
	*/
public:
  worker_node(QObject * pParent = NULL);
};
#endif
