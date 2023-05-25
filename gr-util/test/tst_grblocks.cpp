#include <QTest>
#include "gr-util/grtopblock.h"
#include "gr-util/grsignalpath.h"
#include "gr-util/grproxyblock.h"
#include "gnuradio/analog/sig_source.h"
#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/head.h>
#include <gnuradio/blocks/stream_to_vector.h>
#include <QVector>
#include <QSignalSpy>

#include <gr-util/grproxyblock.h>
#include <gr-util/griiocomplexchannelsrc.h>
#include <gr-util/griiodevicesource.h>
#include <gr-util/griiofloatchannelsrc.h>
#include <gr-util/grscaleoffsetproc.h>
#include <gr-util/grsignalpath.h>
#include <gr-util/grsignalsrc.h>
#include <gr-util/grtopblock.h>

using namespace scopy::grutil;

class TST_GRBlocks : public QObject
{
	Q_OBJECT
private Q_SLOTS: // these are tests
	void test1();
	void test2();
	void test3();
	void test4();
	void test5();

public Q_SLOTS: // these are actual slots
	void connectVectorSinks(); // return vec sinks
	QVector<float> computeSigSourceExpected(gr::analog::gr_waveform_t wave, float ampl, float offset, float sr, float freq, float scale_1, float offset_1);

private:
	void connectVectorSinks(GRTopBlock* top); // return vec sinks
	struct test1Params {
		const int nr_samples = 100;
		const float sig_ampl = 2;
		const float sig_offset = 0;
		const float sig_sr = 100;  // only select an integer multiple of frequency here for testing
		const float sig_freq = 10; //
		const float offset_1 = 2;
		const float scale_1 = 1;

		const float offset_2 = 1;
		const float scale_2 = 2;

	} t1;

	std::vector<gr::blocks::vector_sink_f::sptr> testOutputs;
};

void TST_GRBlocks::connectVectorSinks() {
	GRTopBlock* sender = dynamic_cast<GRTopBlock*>(QObject::sender());
	connectVectorSinks(sender);
}

void TST_GRBlocks::connectVectorSinks(GRTopBlock* top) {

	testOutputs.clear();
	gr::blocks::head::sptr head;
	gr::blocks::stream_to_vector::sptr s2v;
	gr::blocks::vector_sink_f::sptr vec;

	std::vector<gr::blocks::vector_sink_f::sptr> ret;

	for( GRSignalPath* path : top->signalPaths()) {
		if(!path->enabled())
			continue;
		gr::basic_block_sptr endpoint = path->getGrEndPoint();
		head = gr::blocks::head::make(sizeof(float),t1.nr_samples*2-1);
		s2v = gr::blocks::stream_to_vector::make(sizeof(float), t1.nr_samples);
		vec = gr::blocks::vector_sink_f::make(t1.nr_samples);

		top->connect(endpoint, 0, head, 0);
		top->connect(head, 0, s2v, 0);
		top->connect(s2v, 0, vec, 0);
		testOutputs.push_back(vec);
	}	
}

QVector<float> TST_GRBlocks::computeSigSourceExpected(gr::analog::gr_waveform_t wave, float ampl, float offset, float sr, float freq, float scale_1, float offset_1) {
	QVector<float> expected;
	auto period = int(t1.sig_sr / t1.sig_freq);
	auto pol_change = period / 2;

	for(int i=0;i<t1.nr_samples;i++) {
		switch(wave) {
		case gr::analog::GR_CONST_WAVE:
			expected.push_back((ampl+offset) * scale_1+ offset_1);
			break;
		case gr::analog::GR_SQR_WAVE:
			if( (i%period) < pol_change)
				expected.push_back(offset * scale_1 + offset_1);
			else
				expected.push_back((ampl+offset) * scale_1 + offset_1);
			break;
		default:
			qWarning()<< "Waveform NOT SUPPORTED";
			break;
		}
	}
	return expected;
}

void TST_GRBlocks::test1() {

	qInfo() << "This tests the basic functionality of GRBlocks ";
	qInfo() << "We create a signal path with a simple source, scale and offset, run it, and verify results";
	qInfo() << "We then destroy the top block, disable the scale and offset, and rerun the flowgraph";

	GRTopBlock top("aa", this);
	GRSignalPath *ch1;
	GRSignalSrc *sin1;
	GRScaleOffsetProc *scale_offset;

	ch1 = top.addSignalPath("iio1");
	sin1 = new GRSignalSrc(ch1);
	scale_offset = new GRScaleOffsetProc(ch1);
	sin1->setWaveform(gr::analog::GR_CONST_WAVE);
	sin1->setSamplingFreq(t1.sig_sr);
	sin1->setAmplitude(t1.sig_ampl);
	sin1->setOffset(t1.sig_offset);

	scale_offset->setScale(t1.scale_1);

	ch1->append(sin1);
	ch1->append(scale_offset);
	top.build();
	qInfo() << "built flowgraph";
	{
		connectVectorSinks(&top);
		sin1->setFreq(t1.sig_freq);             // change parameters after building
		scale_offset->setOffset(t1.offset_1);
		qInfo() << "modified scale_offset after build";
		top.getGrBlock()->run();

		// |sig_source| --> |multiply| --> |add| --> |head| --> |stream_to_vector| --> |vector_sink|
		QCOMPARE(QString::fromStdString(top.getGrBlock()->edge_list()),
				 QString("multiply_const_ff0:0->add_const_ff0:0\n" // Build order matters for edge_list
						 "sig_source0:0->multiply_const_ff0:0\n"
						 "add_const_ff0:0->head0:0\n"
						 "head0:0->stream_to_vector0:0\n"
						 "stream_to_vector0:0->vector_sink0:0\n"));


		QVector<float> expected = computeSigSourceExpected(gr::analog::GR_CONST_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq,t1.scale_1,t1.offset_1);
		std::vector<float> data = testOutputs[0]->data();
		QVector<float> res = QVector<float>(data.begin(),data.end());
		QCOMPARE(res, expected);
	}

	top.teardown();
	scale_offset->setEnabled(false); // disabling requires rebuild - should this be handled internally (?)
	qInfo()<<"disabled block in signal path";
	top.build();
	{
		connectVectorSinks(&top);
		top.getGrBlock()->run();
		// |sig_source| --> |head| --> |stream_to_vector| --> |vector_sink|
		QCOMPARE(QString::fromStdString(top.getGrBlock()->edge_list()),
				 QString("sig_source0:0->head0:0\n"
						 "head0:0->stream_to_vector0:0\n"
						 "stream_to_vector0:0->vector_sink0:0\n"));


		QVector<float> expected = computeSigSourceExpected(gr::analog::GR_CONST_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, 1, 0);
		std::vector<float> data = testOutputs[0]->data();
		QVector<float> res = QVector<float>(data.begin(),data.end());
		qDebug()<<res;

		qDebug()<<expected;
		QCOMPARE(res, expected);

	}
}

void TST_GRBlocks::test2() {
	qInfo() << "This testcase verifies if one can use a signal path as a source for a different signal path";

	GRTopBlock top("aa", this);
	GRSignalPath *ch1;
	GRSignalPath *ch2;
	GRSignalSrc *sin1;
	GRScaleOffsetProc *scale_offset;

	ch1 = new GRSignalPath("iio1",&top);//top.addSignalPath("iio1");
	sin1 = new GRSignalSrc(ch1);

	ch2 = top.addSignalPath("iio2");
	scale_offset = new GRScaleOffsetProc(ch2);

	sin1->setWaveform(gr::analog::GR_CONST_WAVE);
	sin1->setSamplingFreq(t1.sig_sr);
	sin1->setAmplitude(t1.sig_ampl);
	sin1->setFreq(t1.sig_freq);
	scale_offset->setOffset(t1.offset_1);
	scale_offset->setScale(t1.scale_1);

	ch1->append(sin1);
	ch2->append(ch1);
	ch2->append(scale_offset);

	top.build();
	qInfo() << "built flowgraph";
	{
		connectVectorSinks(&top);

		top.getGrBlock()->run();

		QCOMPARE(QString::fromStdString(top.getGrBlock()->edge_list()),
				 QString("multiply_const_ff0:0->add_const_ff0:0\n"
						 "sig_source0:0->multiply_const_ff0:0\n"
						 "add_const_ff0:0->head0:0\n"
						 "head0:0->stream_to_vector0:0\n"
						 "stream_to_vector0:0->vector_sink0:0\n"));


		QVector<float> expected = computeSigSourceExpected(gr::analog::GR_CONST_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, t1.scale_1, t1.offset_1);

		std::vector<float> data = testOutputs[0]->data();
		QVector<float> res = QVector<float>(data.begin(),data.end());

		QCOMPARE(res, expected);
	}
}

void TST_GRBlocks::test3() {

	qInfo() << "This testcase verifies if multiple signalpaths work for the same topblock";
	GRTopBlock top("aa", this);
	GRSignalPath *ch1,*ch2,*ch3;
	GRSignalSrc *sin1, *sin2;
	GRScaleOffsetProc *scale_offset_1;
	GRScaleOffsetProc *scale_offset_2;

	ch1 = top.addSignalPath("iio1");
	ch2 = top.addSignalPath("iio2");
	ch3 = top.addSignalPath("iio3");

	sin1 = new GRSignalSrc(ch1);
	sin2 = new GRSignalSrc(ch2);

	scale_offset_1 = new GRScaleOffsetProc(ch2);
	scale_offset_2 = new GRScaleOffsetProc(ch3);


	sin1->setWaveform(gr::analog::GR_CONST_WAVE);
	sin1->setSamplingFreq(t1.sig_sr);
	sin1->setAmplitude(t1.sig_ampl);
	sin1->setFreq(t1.sig_freq);

	sin2->setWaveform(gr::analog::GR_SQR_WAVE);
	sin2->setSamplingFreq(t1.sig_sr);
	sin2->setAmplitude(t1.sig_ampl);
	sin2->setFreq(t1.sig_freq);

	scale_offset_1->setOffset(t1.offset_1);
	scale_offset_1->setScale(t1.scale_1);

	scale_offset_2->setOffset(t1.offset_2);
	scale_offset_2->setScale(t1.scale_2);


	/*   |sin1| --+------------------------- - ch1
	 *            +---|scale_offset_1|------ - ch2
	 *   |sin2| ------|scale_offset_2|------ - ch3
	 */
	ch1->append(sin1);
	ch2->append(ch1);
	ch2->append(scale_offset_1);
	ch3->append(sin2);
	ch3->append(scale_offset_2);

	top.build();
	qInfo() << "built flowgraph";
	{
		connectVectorSinks(&top);
		top.getGrBlock()->run();

//		qDebug()<<QString::fromStdString(top.getGrBlock()->edge_list());
		QCOMPARE(QString::fromStdString(top.getGrBlock()->edge_list()),    // where is sig_source_0? // leaked from prev testcase ..
				 QString("multiply_const_ff0:0->add_const_ff0:0\n"
						 "sig_source0:0->multiply_const_ff0:0\n"
						 "multiply_const_ff1:0->add_const_ff1:0\n"
						 "sig_source1:0->multiply_const_ff1:0\n"
						 "sig_source0:0->head0:0\n"
						 "head0:0->stream_to_vector0:0\n"
						 "stream_to_vector0:0->vector_sink0:0\n"
						 "add_const_ff0:0->head1:0\n"
						 "head1:0->stream_to_vector1:0\n"
						 "stream_to_vector1:0->vector_sink1:0\n"
						 "add_const_ff1:0->head2:0\n"
						 "head2:0->stream_to_vector2:0\n"
						 "stream_to_vector2:0->vector_sink2:0\n"));


		QVector<QVector<float>> expectedAll;
		// constant no scale / offset block
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_CONST_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, 1, 0));
		// constant with scale and offset
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_CONST_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, t1.scale_1, t1.offset_1));
		// third channel square wave
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_SQR_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, t1.scale_2, t1.offset_2));

		for(int i = 0; i < testOutputs.size();i++) {
			std::vector<float> data = testOutputs[i]->data();
			QVector<float> res = QVector<float>(data.begin(),data.end());
			qDebug()<<expectedAll[i];
			qDebug()<<testOutputs[i]->data();
			QCOMPARE(res, expectedAll[i]);
		}
	}
	top.teardown();
	ch2->setEnabled(false);
	scale_offset_2->setEnabled(false);
	top.build();

	{
		connectVectorSinks(&top);
		top.getGrBlock()->run();
		QCOMPARE(QString::fromStdString(top.getGrBlock()->edge_list()),
				 QString("sig_source0:0->head0:0\n"
						 "head0:0->stream_to_vector0:0\n"
						 "stream_to_vector0:0->vector_sink0:0\n"
						 "sig_source1:0->head1:0\n"
						 "head1:0->stream_to_vector1:0\n"
						 "stream_to_vector1:0->vector_sink1:0\n"));


		QVector<QVector<float>> expectedAll;

		// constant no scale / offset block
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_CONST_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, 1, 0));
		// constant with scale and offset
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_SQR_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, 1, 0));

		for(int i = 0; i < testOutputs.size();i++) {

			std::vector<float> data = testOutputs[i]->data();
			QVector<float> res = QVector<float>(data.begin(),data.end());

			qDebug()<<expectedAll[i];
			qDebug()<<testOutputs[i]->data();

			QCOMPARE(res, expectedAll[i]);
		}
	}

	top.teardown();


	qInfo()<<"This test verifies that a source can be accessed indirectly";

	ch1->setEnabled(false); // disable first signal path
	ch2->setEnabled(true); // second signal path should use the first source indirectly

	scale_offset_2->setEnabled(false);
	top.build();

	{
		connectVectorSinks(&top);

		top.getGrBlock()->run();
		qDebug()<<QString::fromStdString(top.getGrBlock()->edge_list());
		QCOMPARE(QString::fromStdString(top.getGrBlock()->edge_list()),
			 QString(
			     "multiply_const_ff0:0->add_const_ff0:0\n"
			     "sig_source0:0->multiply_const_ff0:0\n"
			     "add_const_ff0:0->head0:0\n"
			     "head0:0->stream_to_vector0:0\n"
			     "stream_to_vector0:0->vector_sink0:0\n"
			     "sig_source1:0->head1:0\n"
			     "head1:0->stream_to_vector1:0\n"
			     "stream_to_vector1:0->vector_sink1:0\n"));


		QVector<QVector<float>> expectedAll;

		// constant no scale / offset block
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_CONST_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, t1.scale_1, t1.offset_1));
		// constant with scale and offset
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_SQR_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, 1, 0));

		for(int i = 0; i < testOutputs.size();i++) {

			std::vector<float> data = testOutputs[i]->data();
			QVector<float> res = QVector<float>(data.begin(),data.end());

			qDebug()<<expectedAll[i];
			qDebug()<<testOutputs[i]->data();

			QCOMPARE(res, expectedAll[i]);
		}
	}
}


void TST_GRBlocks::test4() {

	qInfo() << "This testcase verifies signal emission on building/teardown and how it can be leveraged to build "
		   "sinks on demand";
	GRTopBlock top("aa", this);
	GRSignalPath *ch1,*ch2,*ch3;
	GRSignalSrc *sin1, *sin2;
	GRScaleOffsetProc *scale_offset_1;
	GRScaleOffsetProc *scale_offset_2;

	ch1 = top.addSignalPath("iio1");
	ch2 = top.addSignalPath("iio2");
	ch3 = top.addSignalPath("iio3");

	sin1 = new GRSignalSrc(ch1);
	sin2 = new GRSignalSrc(ch2);

	scale_offset_1 = new GRScaleOffsetProc(ch2);
	scale_offset_2 = new GRScaleOffsetProc(ch3);


	sin1->setWaveform(gr::analog::GR_CONST_WAVE);
	sin1->setSamplingFreq(t1.sig_sr);
	sin1->setAmplitude(t1.sig_ampl);
	sin1->setFreq(t1.sig_freq);

	sin2->setWaveform(gr::analog::GR_SQR_WAVE);
	sin2->setSamplingFreq(t1.sig_sr);
	sin2->setAmplitude(t1.sig_ampl);
	sin2->setFreq(t1.sig_freq);

	scale_offset_1->setOffset(t1.offset_1);
	scale_offset_1->setScale(t1.scale_1);

	scale_offset_2->setOffset(t1.offset_2);
	scale_offset_2->setScale(t1.scale_2);


	/*   |sin1| --+------------------------- - ch1
	 *            +---|scale_offset_1|------ - ch2
	 *   |sin2| ------|scale_offset_2|------ - ch3
	 */
	ch1->append(sin1);
	ch2->append(ch1);
	ch2->append(scale_offset_1);
	ch3->append(sin2);
	ch3->append(scale_offset_2);

	connect(&top,SIGNAL(builtSignalPaths()), this, SLOT(connectVectorSinks()));
	top.build();
	top.start();

	{
		top.getGrBlock()->wait(); // for testing purposes

		qDebug()<<QString::fromStdString(top.getGrBlock()->edge_list());
		QCOMPARE(QString::fromStdString(top.getGrBlock()->edge_list()),    // where is sig_source_0? // leaked from prev testcase ..
			 QString("multiply_const_ff0:0->add_const_ff0:0\n"
				 "sig_source0:0->multiply_const_ff0:0\n"
				 "multiply_const_ff1:0->add_const_ff1:0\n"
				 "sig_source1:0->multiply_const_ff1:0\n"
				 "sig_source0:0->head0:0\n"
				 "head0:0->stream_to_vector0:0\n"
				 "stream_to_vector0:0->vector_sink0:0\n"
				 "add_const_ff0:0->head1:0\n"
				 "head1:0->stream_to_vector1:0\n"
				 "stream_to_vector1:0->vector_sink1:0\n"
				 "add_const_ff1:0->head2:0\n"
				 "head2:0->stream_to_vector2:0\n"
				 "stream_to_vector2:0->vector_sink2:0\n"));


		QVector<QVector<float>> expectedAll;
		// constant no scale / offset block
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_CONST_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, 1, 0));
		// constant with scale and offset
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_CONST_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, t1.scale_1, t1.offset_1));
		// third channel square wave
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_SQR_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, t1.scale_2, t1.offset_2));

		for(int i = 0; i < testOutputs.size();i++) {
			std::vector<float> data = testOutputs[i]->data();
			QVector<float> res = QVector<float>(data.begin(),data.end());
			qDebug()<<expectedAll[i];
			qDebug()<<testOutputs[i]->data();
			QCOMPARE(res, expectedAll[i]);
		}
	}

	QSignalSpy spy(&top,SIGNAL(builtSignalPaths()));
	ch2->setEnabled(false);
	scale_offset_2->setEnabled(false);
	QCOMPARE(spy.count(),2); // flowgraph rebuilt twice
	{
		top.getGrBlock()->wait(); // for testing purposes
		QCOMPARE(QString::fromStdString(top.getGrBlock()->edge_list()),
			 QString("sig_source0:0->head0:0\n"
				 "head0:0->stream_to_vector0:0\n"
				 "stream_to_vector0:0->vector_sink0:0\n"
				 "sig_source1:0->head1:0\n"
				 "head1:0->stream_to_vector1:0\n"
				 "stream_to_vector1:0->vector_sink1:0\n"));


		QVector<QVector<float>> expectedAll;

		// constant no scale / offset block
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_CONST_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, 1, 0));
		// constant with scale and offset
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_SQR_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, 1, 0));

		for(int i = 0; i < testOutputs.size();i++) {

			std::vector<float> data = testOutputs[i]->data();
			QVector<float> res = QVector<float>(data.begin(),data.end());

			qDebug()<<expectedAll[i];
			qDebug()<<testOutputs[i]->data();

			QCOMPARE(res, expectedAll[i]);
		}
	}
	top.stop();
	top.teardown();
	QSignalSpy spy2(&top,SIGNAL(builtSignalPaths()));
	ch2->setEnabled(true);
	scale_offset_2->setEnabled(true);
	QCOMPARE(spy2.count(),0); // flowgraph is not rebuilt because it was not built
	top.build();
	top.start();
	QCOMPARE(spy2.count(),1); // built only once

	{
		top.getGrBlock()->wait(); // for testing purposes

		qDebug()<<QString::fromStdString(top.getGrBlock()->edge_list());
		QCOMPARE(QString::fromStdString(top.getGrBlock()->edge_list()),    // where is sig_source_0? // leaked from prev testcase ..
			 QString("multiply_const_ff0:0->add_const_ff0:0\n"
				 "sig_source0:0->multiply_const_ff0:0\n"
				 "multiply_const_ff1:0->add_const_ff1:0\n"
				 "sig_source1:0->multiply_const_ff1:0\n"
				 "sig_source0:0->head0:0\n"
				 "head0:0->stream_to_vector0:0\n"
				 "stream_to_vector0:0->vector_sink0:0\n"
				 "add_const_ff0:0->head1:0\n"
				 "head1:0->stream_to_vector1:0\n"
				 "stream_to_vector1:0->vector_sink1:0\n"
				 "add_const_ff1:0->head2:0\n"
				 "head2:0->stream_to_vector2:0\n"
				 "stream_to_vector2:0->vector_sink2:0\n"));


		QVector<QVector<float>> expectedAll;
		// constant no scale / offset block
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_CONST_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, 1, 0));
		// constant with scale and offset
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_CONST_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, t1.scale_1, t1.offset_1));
		// third channel square wave
		expectedAll.push_back(computeSigSourceExpected(gr::analog::GR_SQR_WAVE,t1.sig_ampl,t1.sig_offset,t1.sig_sr,t1.sig_freq, t1.scale_2, t1.offset_2));

		for(int i = 0; i < testOutputs.size();i++) {
			std::vector<float> data = testOutputs[i]->data();
			QVector<float> res = QVector<float>(data.begin(),data.end());
			qDebug()<<expectedAll[i];
			qDebug()<<testOutputs[i]->data();
			QCOMPARE(res, expectedAll[i]);
		}
	}


}

void TST_GRBlocks::test5() {
	// test iio-source
	// create iio-source (one channel)
	// create iio-source (two channel)
	// create iio-source (two-channel inverted)
	// create iio-source (complex)
}




// tests:
// figure out lifecycle for build/connect/disconnect/teardown - just getEndPoint - and build if required - all goes recursively (?) - QoL change - not necessary rn

// add more blocks (?)
// - dc blocker
// - soft trigger
// - head


// add iio-test
// add math-test
// add audio-test
// add file-test


QTEST_MAIN(TST_GRBlocks)
//int main() {

//}

#include "tst_grblocks.moc"


