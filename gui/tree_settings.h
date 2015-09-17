/*******************************************************************************
 *
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 *
 * This software can be redistributed and/or modified freely provided that
 * any derivative works bear some notice that they are derived from it, and
 * any modified versions bear some notice that they have been modified.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      TableChanSettings - table for displaying and manipulating Pixie's
 *      channel settings and chosing detectors.
 *
 ******************************************************************************/


#ifndef TREE_SETTINGS_H_
#define TREE_SETTINGS_H_

#include <QAbstractTableModel>
#include <QAbstractItemModel>
#include <QFont>
#include <QBrush>
#include "generic_setting.h"

class TreeItem
{
public:
    explicit TreeItem(const Gamma::Setting &data, TreeItem *parent = 0);
    ~TreeItem();

    void eat_data(const Gamma::Setting &data);

    TreeItem *child(int number);
    int childCount() const;
    int columnCount() const;
    QVariant display_data(int column) const;
    QVariant edit_data(int column) const;
    bool is_editable(int column) const;
//    bool insertChildren(int position, int count, int columns);
    TreeItem *parent();
//    bool removeChildren(int position, int count);
    int childNumber() const;
    bool setData(int column, const QVariant &value);
    Gamma::Setting rebuild();

private:
    QVector<TreeItem*> childItems;
    Gamma::Setting itemData;
    TreeItem *parentItem;
};


class TreeSettings : public QAbstractItemModel
{
  Q_OBJECT

private:
  Gamma::Setting data_;
  TreeItem *getItem(const QModelIndex &index) const;

  TreeItem *rootItem;

public:  
  explicit TreeSettings(QObject *parent = 0);
  ~TreeSettings();

  QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
  Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
  QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
  int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
  bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;

//  bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
//  bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;

  const Gamma::Setting & get_tree();

  void update(const Gamma::Setting &data);
  void set_structure(const Gamma::Setting &data);

signals:
  void tree_changed();
  void detector_chosen(int chan, std::string name);

};

#endif