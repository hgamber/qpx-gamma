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
 *      FormEnergyCalibration - 
 *
 ******************************************************************************/

#include "form_energy_calibration.h"
#include "widget_detectors.h"
#include "ui_form_energy_calibration.h"
#include "fitter.h"
#include "qt_util.h"
#include <QSettings>


FormEnergyCalibration::FormEnergyCalibration(XMLableDB<Qpx::Detector>& dets, Qpx::Fitter &fit, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FormEnergyCalibration),
  detectors_(dets),
  fit_data_(fit)
{
  ui->setupUi(this);

  loadSettings();

  QColor point_color;
  point_color.setHsv(180, 215, 150, 120);
  style_pts.default_pen = QPen(point_color, 9);
  QColor selected_color;
  selected_color.setHsv(225, 255, 230, 210);
  style_pts.themes["selected"] = QPen(selected_color, 9);
  style_fit.default_pen = QPen(Qt::darkCyan, 2);

  ui->PlotCalib->setAxisLabels("channel", "energy");


  ui->tablePeaks->verticalHeader()->hide();
  ui->tablePeaks->setColumnCount(3);
  ui->tablePeaks->setHorizontalHeaderLabels({"chan", "err(chan)", "energy"});
  ui->tablePeaks->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tablePeaks->setSelectionMode(QAbstractItemView::ExtendedSelection);

  //all columns?
  ui->tablePeaks->setEditTriggers(QTableView::NoEditTriggers);

  ui->tablePeaks->horizontalHeader()->setStretchLastSection(true);
  ui->tablePeaks->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->tablePeaks->show();
  connect(ui->tablePeaks, SIGNAL(itemSelectionChanged()), this, SLOT(selection_changed_in_table()));

  connect(ui->PlotCalib, SIGNAL(selection_changed()), this, SLOT(selection_changed_in_plot()));

  ui->isotopes->show();
  connect(ui->isotopes, SIGNAL(energiesSelected()), this, SLOT(isotope_energies_chosen()));

}

FormEnergyCalibration::~FormEnergyCalibration()
{
  delete ui;
}

bool FormEnergyCalibration::save_close() {
  if (ui->isotopes->save_close()) {
    saveSettings();
    return true;
  }
  else
    return false;
}

void FormEnergyCalibration::loadSettings() {
  QSettings settings_;

  settings_.beginGroup("Program");
  settings_directory_ = settings_.value("settings_directory", QDir::homePath() + "/qpx/settings").toString();
  data_directory_ = settings_.value("save_directory", QDir::homePath() + "/qpx/data").toString();
  settings_.endGroup();

  ui->isotopes->setDir(settings_directory_);

  settings_.beginGroup("Energy_calibration");
  ui->spinTerms->setValue(settings_.value("fit_function_terms", 2).toInt());
  ui->isotopes->set_current_isotope(settings_.value("current_isotope", "Co-60").toString());

  settings_.endGroup();

}

void FormEnergyCalibration::saveSettings() {
  QSettings settings_;

  settings_.beginGroup("Energy_calibration");
  settings_.setValue("fit_function_terms", ui->spinTerms->value());
  settings_.setValue("current_isotope", ui->isotopes->current_isotope());
  settings_.endGroup();
}

void FormEnergyCalibration::clear()
{
  new_calibration_ = Qpx::Calibration();
  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(0);
  toggle_push();
  ui->PlotCalib->clearAll();
  ui->PlotCalib->replot();
  ui->pushApplyCalib->setEnabled(false);
  ui->pushFromDB->setEnabled(false);
}

void FormEnergyCalibration::newSpectrum() {
  new_calibration_ = fit_data_.settings().cali_nrg_;
  update_data();
}

void FormEnergyCalibration::update_data() {
  rebuild_table();
  replot_calib();

  if (fit_data_.peaks().empty())
    selected_peaks_.clear();

  select_in_table();
  select_in_plot();
  toggle_push();
}

void FormEnergyCalibration::update_selection(std::set<double> selected_peaks) {
  bool changed = (selected_peaks_ != selected_peaks);
  selected_peaks_ = selected_peaks;

  if (changed) {
    select_in_table();
    select_in_plot();
  }
}

void FormEnergyCalibration::select_in_table() {
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);
  ui->tablePeaks->clearSelection();

  QItemSelectionModel *selectionModel = ui->tablePeaks->selectionModel();
  QItemSelection itemSelection = selectionModel->selection();

  for (int i=0; i < ui->tablePeaks->rowCount(); ++i)
    if (selected_peaks_.count(ui->tablePeaks->item(i, 0)->data(Qt::UserRole).toDouble())) {
      ui->tablePeaks->selectRow(i);
      itemSelection.merge(selectionModel->selection(), QItemSelectionModel::Select);
    }

  selectionModel->clearSelection();
  selectionModel->select(itemSelection,QItemSelectionModel::Select);

  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}

void FormEnergyCalibration::replot_calib() {
  ui->PlotCalib->clearAll();

  QVector<double> xx, yy;

  double xmin = std::numeric_limits<double>::max();
  double xmax = - std::numeric_limits<double>::max();

  for (auto &q : fit_data_.peaks()) {
    double x = q.first;
    double y = q.second.energy().value();

    xx.push_back(x);
    yy.push_back(y);

    if (x < xmin)
      xmin = x;
    if (x > xmax)
      xmax = x;
  }

  double x_margin = (xmax - xmin) / 10;
  xmax += x_margin;
  xmin -= x_margin;

  if (xx.size()) {
    QVector<double> xx_sigma(yy.size(), 0);
    QVector<double> yy_sigma(yy.size(), 0);

    ui->PlotCalib->addPoints(style_pts, xx, yy, xx_sigma, yy_sigma);
    ui->PlotCalib->set_selected_pts(selected_peaks_);
    if (new_calibration_.valid())
    {
      double step = (xmax-xmin) / 50.0;
      xx.clear(); yy.clear();

      for (double i=xmin; i < xmax; i+=step)
      {
        xx.push_back(i);
        yy.push_back(new_calibration_.transform(i));
      }
      ui->PlotCalib->setFit(xx, yy, style_fit);
      ui->PlotCalib->setTitle("E = " + QString::fromStdString(new_calibration_.fancy_equation(6, true)));
    }
  }

  ui->PlotCalib->replotAll();
}

void FormEnergyCalibration::rebuild_table() {
  ui->tablePeaks->blockSignals(true);
  this->blockSignals(true);

  std::set<double> flagged;
  ui->tablePeaks->clearContents();
  ui->tablePeaks->setRowCount(fit_data_.peaks().size());
  int i=0;
  for (auto &p : fit_data_.peaks()) {
    bool close = false;
    for (auto &e : ui->isotopes->current_isotope_gammas())
      if (abs(p.second.energy().value() - e.energy) < 2.0) {//hardcoded
        flagged.insert(e.energy);
        close = true;
      }
    add_peak_to_table(p.second, i, close);
    ++i;
  }

//  ui->isotopes->select_energies(flagged);

  ui->tablePeaks->blockSignals(false);
  this->blockSignals(false);
}

void FormEnergyCalibration::selection_changed_in_plot()
{
  selected_peaks_ = ui->PlotCalib->get_selected_pts();
  select_in_table();
  if (isVisible())
    emit selection_changed(selected_peaks_);
  toggle_push();
}

void FormEnergyCalibration::selection_changed_in_table() {
  selected_peaks_.clear();
  foreach (QModelIndex i, ui->tablePeaks->selectionModel()->selectedRows())
    selected_peaks_.insert(ui->tablePeaks->item(i.row(), 0)->data(Qt::UserRole).toDouble());

  select_in_plot();
  if (isVisible())
    emit selection_changed(selected_peaks_);
  toggle_push();
}

void FormEnergyCalibration::toggle_push() {
  size_t sel = selected_peaks_.size();

  ui->pushEnergiesToPeaks->setEnabled((sel > 0) && (sel == ui->isotopes->current_gammas().size()));
  ui->pushPeaksToNuclide->setEnabled((sel > 0) && (ui->isotopes->current_gammas().empty()));

  if (static_cast<int>(fit_data_.peaks().size()) > 1) {
    ui->pushFit->setEnabled(true);
    ui->spinTerms->setEnabled(true);
  } else {
    ui->pushFit->setEnabled(false);
    ui->spinTerms->setEnabled(false);
  }

  if (detectors_.has_a(fit_data_.detector_) && detectors_.get(fit_data_.detector_).energy_calibrations_.has_a(new_calibration_))
    ui->pushFromDB->setEnabled(true);
  else
    ui->pushFromDB->setEnabled(false);

  ui->pushApplyCalib->setEnabled(fit_data_.settings().cali_nrg_ != new_calibration_);
}

void FormEnergyCalibration::on_pushFit_clicked()
{  
  std::vector<double> x, y;
  x.resize(fit_data_.peaks().size());
  y.resize(fit_data_.peaks().size());
  int i = 0;
  for (auto &q : fit_data_.peaks())
  {
    x[i] = q.first;
    y[i] = q.second.energy().value();
    i++;
  }

//  std::vector<double> sigmas(y.size(), 1);

  PolyBounded p;
  p.add_coeff(0, -50, 50, 0);
  p.add_coeff(1,   0, 50, 1);
  for (int i=2; i <= ui->spinTerms->value(); ++i)
      p.add_coeff(i, -5, 5, 0);
//  PolyBounded p2 = p;

  p.fit(x,y, std::vector<double>(), std::vector<double>());

  #ifdef FITTER_ROOT_ENABLED
//  p2.fit_root(x,y,std::vector<double>(), std::vector<double>());
//  p = p2;
//  p2 = p;
//  p2.fit_root(x,y,sigmas);
  #endif

  if (p.coeffs_.size())
  {
    new_calibration_.type_ = "Energy";
    new_calibration_.bits_ = fit_data_.settings().bits_;
    new_calibration_.coefficients_ = p.coeffs();
    new_calibration_.r_squared_ = p.rsq_;
    new_calibration_.calib_date_ = boost::posix_time::microsec_clock::universal_time();  //spectrum timestamp instead?
    new_calibration_.units_ = "keV";
    new_calibration_.model_ = Qpx::CalibrationModel::polynomial;
  }
  else
    WARN << "<Energy calibration> Qpx::Calibration failed";

  replot_calib();
  select_in_plot();
  toggle_push();
  emit new_fit();
}

void FormEnergyCalibration::isotope_energies_chosen() {
  update_data();
}

void FormEnergyCalibration::on_pushApplyCalib_clicked()
{
  emit update_detector();
}

void FormEnergyCalibration::on_pushFromDB_clicked()
{
  Qpx::Detector newdet = detectors_.get(fit_data_.detector_);
  new_calibration_ = newdet.energy_calibrations_.get(new_calibration_);
  replot_calib();
  select_in_plot();
  toggle_push();
  emit new_fit();
}

void FormEnergyCalibration::on_pushDetDB_clicked()
{
  WidgetDetectors *det_widget = new WidgetDetectors(this);
  det_widget->setData(detectors_, settings_directory_);
  connect(det_widget, SIGNAL(detectorsUpdated()), this, SLOT(detectorsUpdated()));
  det_widget->exec();
}

void FormEnergyCalibration::on_pushPeaksToNuclide_clicked()
{
  std::vector<double> gammas;
  for (auto &q : fit_data_.peaks())
    if (selected_peaks_.count(q.first))
      gammas.push_back(q.second.energy().value());
  ui->isotopes->push_energies(gammas);
}

void FormEnergyCalibration::on_pushEnergiesToPeaks_clicked()
{
  std::vector<double> gammas = ui->isotopes->current_gammas();
  std::sort(gammas.begin(), gammas.end());

  std::vector<double> peakIDs;
  double last_sel = -1;
  for (auto &q : fit_data_.peaks())
    if (selected_peaks_.count(q.first)) {
      peakIDs.push_back(q.first);
      last_sel = q.first;
    }

  if (gammas.size() != peakIDs.size())
    return;

  for (size_t i=0; i<gammas.size(); ++i)
    fit_data_.override_energy(peakIDs.at(i), gammas.at(i));

  selected_peaks_.clear();
  for (auto &q : fit_data_.peaks())
    if (q.first > last_sel) {
      selected_peaks_.insert(q.first);
      break;
    }

  ui->isotopes->select_next_energy();

  update_data();
  emit change_peaks();
  emit selection_changed(selected_peaks_);
}

void FormEnergyCalibration::add_peak_to_table(const Qpx::Peak &p, int row, bool gray) {
  QBrush background(gray ? Qt::lightGray : Qt::white);

  add_to_table(ui->tablePeaks, row, 0, p.center().to_string(),
               QVariant::fromValue(p.center().value()), background);
  add_to_table(ui->tablePeaks, row, 1, p.center().error_percent(), QVariant(), background);
  add_to_table(ui->tablePeaks, row, 2, p.energy().to_string(), QVariant(), background);

}

void FormEnergyCalibration::select_in_plot()
{
  ui->PlotCalib->set_selected_pts(selected_peaks_);
  ui->PlotCalib->replotAll();
}
