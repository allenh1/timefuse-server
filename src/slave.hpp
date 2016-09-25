#ifndef __SLAVE_HPP__
#define __SLAVE_HPP__
/**
 * This class has all attributes relating to a
 * slave node. This is the big task.
 */
class slave_node : public QObject
{
  Q_OBJECT
public:
  slave_node(QObject * pParent = NULL);
};
#endif
