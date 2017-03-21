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
 * Description:
 *      WidgetSaveTypes - widget for selecting file types for saving spectra
 *      DialogSaveSpectra  - dialog for chosing location and types
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 ******************************************************************************/


#include "consumer_factory.h"
#include <algorithm>
#include <QPaintEvent>
#include <QPainter>
#include <QFileInfo>
#include "dialog_save_spectra.h"
#include "ui_dialog_save_spectra.h"
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include "custom_timer.h"

using namespace Qpx;

WidgetSaveTypes::WidgetSaveTypes(QWidget *parent)
  : QWidget(parent),
    max_formats_(0)
{
  setMouseTracking(true);
  setAutoFillBackground(true);
  w_ = 50;
  h_ = 25;
}

void WidgetSaveTypes::initialize(std::vector<std::string> types) {
  if (types.empty())
    return;

  spectrum_types = types;
  file_formats.resize(spectrum_types.size());
  selections.resize(spectrum_types.size());
  for (std::size_t i = 0; i < spectrum_types.size(); i++) {
    ConsumerMetadata type_template = ConsumerFactory::getInstance().create_prototype(spectrum_types[i]);
    std::list<std::string> otypes = type_template.output_types();
    file_formats[i] = std::vector<std::string>(otypes.begin(), otypes.end());
    selections[i].resize(file_formats[i].size(), false);
    max_formats_ = std::max(max_formats_, static_cast<int>(file_formats[i].size()));
  }
}

QSize WidgetSaveTypes::sizeHint() const
{
  return QSize(max_formats_*w_, spectrum_types.size()*h_);
}

void WidgetSaveTypes::paintEvent(QPaintEvent *evt)
{
  QPainter painter(this);
  QRectF one_rect = QRectF(0, 0, w_, h_);

  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setFont(QFont("Times", 10, QFont::Normal));

  for (std::size_t i = 0; i < file_formats.size(); i++) {

    painter.setBrush(this->palette().background());
    painter.setPen(QPen(this->palette().foreground().color()));
    painter.resetTransform();
    painter.translate(evt->rect().x(), evt->rect().y() + i*h_);
    painter.drawRect(one_rect);
    painter.drawText(one_rect, Qt::AlignCenter, QString::fromStdString(spectrum_types[i]));

    painter.setPen(QPen(Qt::white));
    for(std::size_t j=0; j < file_formats[i].size(); j++){
      if (selections[i][j])
        painter.setBrush(Qt::blue);
      else
        painter.setBrush(Qt::black);

      painter.resetTransform();
      painter.translate(evt->rect().x() + (j+1)*w_, evt->rect().y() + i*h_);
      painter.drawRect(one_rect);
      painter.drawText(one_rect, Qt::AlignCenter, QString("*.") + QString::fromStdString(file_formats[i][j]));
    }
  }
}

void WidgetSaveTypes::mouseReleaseEvent(QMouseEvent *event)
{
  std::size_t phigh = event->y() / h_;
  std::size_t pwide = (event->x() / w_) - 1;
  if ((phigh >= spectrum_types.size()) || (pwide >= file_formats[phigh].size()))
    return;

  selections[phigh][pwide] = !selections[phigh][pwide];
  update();
  emit stateChanged();
}



////Dialog///////////////

DialogSaveSpectra::DialogSaveSpectra(Project& newset, QString outdir, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DialogSaveSpectra)
{
  my_set_ = &newset;

  ui->setupUi(this);
  ui->typesWidget->initialize(my_set_->types());

  root_dir_ = outdir;
  std::string timenow = boost::posix_time::to_iso_string(boost::posix_time::second_clock::universal_time());

  QFileInfo fi(QString::fromStdString(my_set_->identity()));

  if (my_set_->identity() != "New project")
    ui->lineName->setText(QString("Qpx_") + fi.baseName());
  else
    ui->lineName->setText(QString("Qpx_") + QString::fromStdString(timenow));
}

DialogSaveSpectra::~DialogSaveSpectra()
{
  delete ui;
}

void DialogSaveSpectra::on_lineName_textChanged(const QString &arg1)
{
  namespace fs = boost::filesystem;
  fs::path dir(root_dir_.toStdString());
  dir.make_preferred();
  dir /= boost::algorithm::trim_copy(arg1.toStdString());

  total_dir_ = dir.string();
  QString total = QString::fromStdString(total_dir_);

  if (fs::is_directory(dir)) {
    total = "<font color='red'>" + total + " already exists </font>";
    ui->buttonBox->setEnabled(false);
  } else if (!fs::portable_directory_name(arg1.toStdString())) {
    total = "<font color='red'>" + total + " invalid name </font>";
    ui->buttonBox->setEnabled(false);
  } else
    ui->buttonBox->setEnabled(true);

  ui->labelDirectory->setText(total);
}

void DialogSaveSpectra::on_buttonBox_accepted()
{
  boost::filesystem::path dir(total_dir_);
  if (!boost::filesystem::create_directory(dir))
  {
    ERR << "Error creating directory";
    emit accepted();
    return;
  }

  CustomTimer filetime(true);

  for (std::size_t i = 0; i < ui->typesWidget->spectrum_types.size(); i ++) {
    std::map<int64_t, SinkPtr> thistype = my_set_->get_sinks(ui->typesWidget->spectrum_types[i]);
    for (std::size_t j = 0; j < ui->typesWidget->file_formats[i].size(); j++) {
      if (ui->typesWidget->selections[i][j]) {
        LINFO << "Saving " << ui->typesWidget->spectrum_types[i] << " spectra as " << ui->typesWidget->file_formats[i][j];
        for (auto &q : thistype) {
          if ((!ui->checkVisibleOnly->isChecked()) || q.second->metadata().get_attribute("visible").value_int)
            q.second->write_file(dir.string(), ui->typesWidget->file_formats[i][j]);
        }
      }
    }
  }


  filetime.stop();

  LINFO << "File writing time " << filetime.s() << " sec";
  emit accepted();
}

void DialogSaveSpectra::on_buttonBox_rejected()
{
  emit accepted();
}
