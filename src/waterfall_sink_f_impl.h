/* -*- c++ -*- */
/*
 * Copyright 2012,2015,2019 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
/*
 * Copyright (c) 2022 Analog Devices Inc.
 *
 * This file is part of Scopy
 * (see http://www.github.com/analogdevicesinc/scopy).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_QTGUI_WATERFALL_SINK_F_IMPL_H
#define INCLUDED_QTGUI_WATERFALL_SINK_F_IMPL_H

#include "WaterfallDisplayPlot.h"
#include "waterfall_sink_f.h"

#include <gnuradio/fft/fft.h>
#include <gnuradio/fft/fft_shift.h>
#include <gnuradio/fft/window.h>
#include <gnuradio/high_res_timer.h>

using namespace gr;
namespace adiscope {

class waterfall_sink_f_impl : public waterfall_sink_f
{
private:
    void forecast(int noutput_items, gr_vector_int& ninput_items_required) override;

    void initialize();

    int d_fftsize;
    gr::fft::fft_shift<float> d_fft_shift;
    float d_fftavg;
    std::vector<float> d_window;
    double d_center_freq;
    double d_bandwidth;
    const std::string d_name;
    int d_nconnections;
    int d_nrows;

    const pmt::pmt_t d_port;
    const pmt::pmt_t d_port_bw;

    // Perform fftshift operation;
    // this is usually desired when plotting
    std::unique_ptr<gr::fft::fft_complex_fwd> d_fft;

    int d_index = 0;
    std::vector<volk::vector<float>> d_residbufs;
    std::vector<volk::vector<double>> d_magbufs;
    double* d_pdu_magbuf;
    volk::vector<float> d_fbuf;

    // Required now for Qt; argc must be greater than 0 and argv
    // must have at least one valid character. Must be valid through
    // life of the qApplication:
    // http://harmattan-dev.nokia.com/docs/library/html/qt4/qapplication.html
    char d_zero = 0;
    int d_argc = 1;
    char* d_argv = &d_zero;
    WaterfallDisplayPlot* d_main_gui = nullptr;

    gr::high_res_timer_type d_update_time;
    gr::high_res_timer_type d_last_time;

    void fftresize();
    void resize_bufs(int size);
    void check_clicked();
    void fft(float* data_out, const float* data_in, int size);

    // Handles message input port for setting new bandwidth
    // The message is a PMT pair (intern('bw'), double(bw))
    void handle_set_bw(pmt::pmt_t msg);

    // Handles message input port for setting new center frequency.
    // The message is a PMT pair (intern('freq'), double(frequency)).
    void handle_set_freq(pmt::pmt_t msg);

    // Handles message input port for displaying PDU samples.
    void handle_pdus(pmt::pmt_t msg);

public:
    waterfall_sink_f_impl(int size,
			  std::vector<float> win,
			  double fc,
			  double bw,
			  const std::string& name,
			  int nconnections,
			  WaterfallDisplayPlot* plot = NULL);
    ~waterfall_sink_f_impl();

    bool check_topology(int ninputs, int noutputs) override;

    void exec_() override;
    QWidget* qwidget() override;

    void clear_data() override;

    void set_fft_size(const int fftsize) override;
    int fft_size() const override;
    float fft_average() const override;
    void set_fft_window(const std::vector<float> window) override;

    void set_frequency_range(const double centerfreq, const double bandwidth) override;
    void set_intensity_range(const double min, const double max) override;

    void set_update_time(double t) override;
    void set_time_per_fft(double t) override;
    void set_line_label(unsigned int which, const std::string& label) override;
    void set_line_alpha(unsigned int which, double alpha) override;
    void set_plot_pos_half(bool half) override;

    double line_alpha(unsigned int which) override;

    void set_size(int width, int height) override;

    void auto_scale() override;
    double min_intensity(unsigned int which) override;
    double max_intensity(unsigned int which) override;

    void disable_legend() override;

    int work(int noutput_items,
	     gr_vector_const_void_star& input_items,
	     gr_vector_void_star& output_items) override;
};

} /* namespace adiscope */

#endif /* INCLUDED_QTGUI_WATERFALL_SINK_F_IMPL_H */
