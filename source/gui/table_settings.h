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
 *      TableChanSettings - table for displaying and manipulating
 *      channel settings and chosing detectors.
 *
 ******************************************************************************/

#pragma once

#include <QAbstractTableModel>
#include <QFont>
#include <QBrush>
#include "detector.h"
#include "units.h"

class TableChanSettings : public QAbstractTableModel
{
    Q_OBJECT
private:
    std::vector<Qpx::Detector> channels_;
    Qpx::Setting consolidated_list_;
    std::map<std::string, std::string> preferred_units_;
    bool show_read_only_;

    std::set<std::string> scalable_units_;

public:
    TableChanSettings(QObject *parent = 0);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex & index) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

    void update(const std::vector<Qpx::Detector> &settings);
    void set_show_read_only(bool show_ro);

signals:
    void setting_changed(Qpx::Setting setting);
    void detector_chosen(int chan, std::string name);

};
