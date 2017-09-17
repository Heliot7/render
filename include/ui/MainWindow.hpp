#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <sstream>
#include <fstream> 

#include <QMessageBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidgetItem>

#include "ui_MainWindow.h"
#include "rendering/Render.hpp"
#include "rendering/Sampler.hpp"

#define valueRange 50.0f;
#define rangeAbove 1.0f/8.0f
#define rangeBelow 1.0f/8.0f

struct DefaultParams
{
    double objX, objY, objZ, objRx, objRy, objRz, objSx, objSy, objSz;
    bool AA, AR;
	double azimuth;
	QString elevation, distance;
    int resSample, numSamples;
};

class MainWindow : public QMainWindow , private Ui::MainWindow
{
    Q_OBJECT

    public:

        MainWindow(QWidget* parent = 0);
        ~MainWindow();

    protected:

        virtual void keyPressEvent(QKeyEvent *event)
        {
            glView->keyPressEvent(event);
            switch(event->key())
            {
                case Qt::Key_F2:
                    if(tabWidget->currentIndex() == 1 && treeLabelling->currentItem() != NULL)
                    {
                        // std::cout << "Enter rename mode!" << std::endl;
						lineNewName->show();
						lineNewName->lower();
						lineNewName->setWindowTitle(QString("Rename:"));
                        connect(lineNewName, SIGNAL(textChanged(const QString&)), this, SLOT(updateLabelName(const QString&)));
                        lineNewName->setText("");
						//lineNewName->activateWindow();
						//lineNewName->hide();
                    }
                    break;
                default:
                    break;

            }
        }
        virtual void keyReleaseEvent(QKeyEvent *event) { glView->keyReleaseEvent(event); }
        virtual void mousePressEvent(QMouseEvent *event) { glView->mousePressEvent(event); }
        virtual void mouseMoveEvent(QMouseEvent *event) { glView->mouseMoveEvent(event); }

    protected slots:

        // VISUALISATION tab

        // Model:
        void on_buttonLoadModel_clicked();
        void on_checkBB_clicked(bool isClicked) { glView->setViewBoundingBox(isClicked); }
        void on_boxX_valueChanged(double newValue) { glView->setPosObj(newValue, boxY->value(), boxZ->value()); }
        void on_boxY_valueChanged(double newValue) { glView->setPosObj(boxX->value(), newValue, boxZ->value()); }
        void on_boxZ_valueChanged(double newValue) { glView->setPosObj(boxX->value(), boxY->value(), newValue); }
        void on_boxRX_valueChanged(double newValue) { glView->setRotObj(newValue, boxRY->value(), boxRZ->value()); }
        void on_boxRY_valueChanged(double newValue) { glView->setRotObj(boxRX->value(), newValue, boxRZ->value()); }
        void on_boxRZ_valueChanged(double newValue) { glView->setRotObj(boxRX->value(), boxRY->value(), newValue); }
        void on_boxSX_valueChanged(double newValue) { if(checkAR->isChecked()) updateScalesAR1(newValue); glView->setScaleObj(newValue, boxSY->value(), boxSZ->value()); }
        void on_boxSY_valueChanged(double newValue) { if(checkAR->isChecked()) updateScalesAR1(newValue); glView->setScaleObj(boxSX->value(), newValue, boxSZ->value()); }
        void on_boxSZ_valueChanged(double newValue) { if(checkAR->isChecked()) updateScalesAR1(newValue); glView->setScaleObj(boxSX->value(), boxSY->value(), newValue); }
        void updateScalesAR1(double newValue)
        {
            if(boxSX->value() != newValue)
                boxSX->setValue(newValue);
            if(boxSY->value() != newValue)
                boxSY->setValue(newValue);
            if(boxSZ->value() != newValue)
                boxSZ->setValue(newValue);
        }
        void on_buttonImageDefault_clicked();
        void on_checkAA_clicked(bool isClicked) { glView->setAntiAliasing(isClicked); }

        // Background:
        void on_buttonLoadBackground_clicked();
        void on_buttonResetBackground_clicked();

        // Camera displacement:
        void on_sliderDistance_valueChanged(int value) { glView->setFixedDistance(value/10.0f); labelDistance->setText(QString("%1%2%3").arg("Distance (").arg(floatToString(value/10.0f).c_str()).arg("):")); }
        void on_sliderElevation_valueChanged(int value) { glView->setFixedElevation(value/10.0f); labelElevation->setText(QString("%1%2%3").arg("Elevation (").arg(floatToString(value/10.0f).c_str()).arg("):")); }
        void on_dialAzimuth_valueChanged(int value) { glView->setFixedAzimuth(value/10.0f); labelAzimuth->setText(QString("%1%2%3").arg("Azimuth (").arg(floatToString(value/10.0f).c_str()).arg("):")); }
        void on_buttonResetCamera_clicked() { glView->resetCamera(); sliderDistance->setValue(21); sliderElevation->setValue(0); dialAzimuth->setValue(0); }
        void on_boxCameraFree_clicked(bool isClicked) { glView->setFreeCamera(isClicked); }

        // Lighting:
        void on_buttonPreviousLight_clicked() { stackedLights->setCurrentIndex((stackedLights->currentIndex()+stackedLights->count()-1) % stackedLights->count()); }
        void on_buttonNextLight_clicked() { stackedLights->setCurrentIndex((stackedLights->currentIndex()+1) % stackedLights->count()); }
		
		// sliders: [-100,100] (100.0f means -8..8 range)
        // - Light #1
        void on_checkLight1_clicked(bool isClicked) { glView->getLights().onLight[0] = isClicked; }
        void on_sliderLight1X_valueChanged() { glView->getLights().position[0].x = (float)sliderLight1X->value()/valueRange; }
        void on_sliderLight1Y_valueChanged() { glView->getLights().position[0].y = (float)sliderLight1Y->value()/valueRange; }
        void on_sliderLight1Z_valueChanged() { glView->getLights().position[0].z = (float)sliderLight1Z->value()/valueRange; }
		void on_sliderLighting1_valueChanged(int newValue)
		{
			float ambInt = (float)newValue / sliderLighting1->maximum() / (Lights::MAX_LIGHTS*rangeAbove);
			glView->getLights().diffuse[0] = glm::vec3(ambInt, ambInt, ambInt);
		}

        // - Light #2
        void on_checkLight2_clicked(bool isClicked) { glView->getLights().onLight[1] = isClicked; }
        void on_sliderLight2X_valueChanged() { glView->getLights().position[1].x = (float)sliderLight2X->value()/valueRange; }
        void on_sliderLight2Y_valueChanged() { glView->getLights().position[1].y = (float)sliderLight2Y->value()/valueRange; }
        void on_sliderLight2Z_valueChanged() { glView->getLights().position[1].z = (float)sliderLight2Z->value()/valueRange; }
		void on_sliderLighting2_valueChanged(int newValue) { float ambInt = (float)newValue / sliderLighting2->maximum() / (Lights::MAX_LIGHTS*rangeAbove); glView->getLights().diffuse[1] = glm::vec3(ambInt, ambInt, ambInt); }
        // - Light #3
        void on_checkLight3_clicked(bool isClicked) { glView->getLights().onLight[2] = isClicked; }
        void on_sliderLight3X_valueChanged() { glView->getLights().position[2].x = (float)sliderLight3X->value()/valueRange; }
        void on_sliderLight3Y_valueChanged() { glView->getLights().position[2].y = (float)sliderLight3Y->value()/valueRange; }
        void on_sliderLight3Z_valueChanged() { glView->getLights().position[2].z = (float)sliderLight3Z->value()/valueRange; }
		void on_sliderLighting3_valueChanged(int newValue) { float ambInt = (float)newValue / sliderLighting3->maximum() / (Lights::MAX_LIGHTS*rangeAbove); glView->getLights().diffuse[2] = glm::vec3(ambInt, ambInt, ambInt); }
        // - Light #4
        void on_checkLight4_clicked(bool isClicked) { glView->getLights().onLight[3] = isClicked; }
        void on_sliderLight4X_valueChanged() { glView->getLights().position[3].x = (float)sliderLight4X->value()/valueRange; }
        void on_sliderLight4Y_valueChanged() { glView->getLights().position[3].y = (float)sliderLight4Y->value()/valueRange; }
        void on_sliderLight4Z_valueChanged() { glView->getLights().position[3].z = (float)sliderLight4Z->value()/valueRange; }
		void on_sliderLighting4_valueChanged(int newValue) { float ambInt = (float)newValue / sliderLighting4->maximum() / (Lights::MAX_LIGHTS*rangeAbove); glView->getLights().diffuse[3] = glm::vec3(ambInt, ambInt, ambInt); }
        // - Light #5
        void on_checkLight5_clicked(bool isClicked) { glView->getLights().onLight[4] = isClicked; }
        void on_sliderLight5X_valueChanged() { glView->getLights().position[4].x = (float)sliderLight5X->value()/valueRange; }
        void on_sliderLight5Y_valueChanged() { glView->getLights().position[4].y = (float)sliderLight5Y->value()/valueRange; }
        void on_sliderLight5Z_valueChanged() { glView->getLights().position[4].z = (float)sliderLight5Z->value()/valueRange; }
		void on_sliderLighting5_valueChanged(int newValue) { float ambInt = (float)newValue / sliderLighting5->maximum() / (Lights::MAX_LIGHTS*rangeAbove); glView->getLights().diffuse[4] = glm::vec3(ambInt, ambInt, ambInt); }
        // - Light #6
        void on_checkLight6_clicked(bool isClicked) { glView->getLights().onLight[5] = isClicked; }
        void on_sliderLight6X_valueChanged() { glView->getLights().position[5].x = (float)sliderLight6X->value()/valueRange; }
        void on_sliderLight6Y_valueChanged() { glView->getLights().position[5].y = (float)sliderLight6Y->value()/valueRange; }
        void on_sliderLight6Z_valueChanged() { glView->getLights().position[5].z = (float)sliderLight6Z->value()/valueRange; }
		void on_sliderLighting6_valueChanged(int newValue) { float ambInt = (float)newValue / sliderLighting6->maximum() / (Lights::MAX_LIGHTS*rangeBelow); glView->getLights().diffuse[5] = glm::vec3(ambInt, ambInt, ambInt); }
        // - Light #7
        void on_checkLight7_clicked(bool isClicked) { glView->getLights().onLight[6] = isClicked; }
        void on_sliderLight7X_valueChanged() { glView->getLights().position[6].x = (float)sliderLight7X->value()/valueRange; }
        void on_sliderLight7Y_valueChanged() { glView->getLights().position[6].y = (float)sliderLight7Y->value()/valueRange; }
        void on_sliderLight7Z_valueChanged() { glView->getLights().position[6].z = (float)sliderLight7Z->value()/valueRange; }
		void on_sliderLighting7_valueChanged(int newValue) { float ambInt = (float)newValue / sliderLighting7->maximum() / (Lights::MAX_LIGHTS*rangeBelow); glView->getLights().diffuse[6] = glm::vec3(ambInt, ambInt, ambInt); }
        // - Light #8
        void on_checkLight8_clicked(bool isClicked) { glView->getLights().onLight[7] = isClicked; }
        void on_sliderLight8X_valueChanged() { glView->getLights().position[7].x = (float)sliderLight8X->value()/valueRange; }
        void on_sliderLight8Y_valueChanged() { glView->getLights().position[7].y = (float)sliderLight8Y->value()/valueRange; }
        void on_sliderLight8Z_valueChanged() { glView->getLights().position[7].z = (float)sliderLight8Z->value()/valueRange; }
		void on_sliderLighting8_valueChanged(int newValue) { float ambInt = (float)newValue / sliderLighting8->maximum() / (Lights::MAX_LIGHTS*rangeBelow); glView->getLights().diffuse[7] = glm::vec3(ambInt, ambInt, ambInt); }
        
        // SEMANTICS tab

        // Labelling:
        void on_tabWidget_currentChanged(int index);
        void updateLabelName(const QString& newLabel);
        void endUpdateLabelName();
        void on_checkLabelling_clicked(bool isClicked);
        void on_buttonAutomaticLabelling_clicked();
        std::vector<unsigned int> getPathNode(QTreeWidgetItem* item);
        void on_buttonNewLabel_clicked();
        void on_buttonDeleteLabel_clicked();
        void on_treeLabelling_itemClicked(QTreeWidgetItem* item, int column = 0);
        void on_treeLabelling_itemDoubleClicked(QTreeWidgetItem* item, int column = 0);
        void on_spinBrushSize_valueChanged(int newValue) { glView->setBrushSize(newValue); }
        void on_buttonSaveLabelling_clicked();
        void saveNodeLabelling(std::ofstream& segmentationFile, QTreeWidgetItem* node);
        void on_buttonLoadLabelling_clicked();
        void updateLabelCompleteness();
		void on_checkPixelMode_clicked(bool isClicked) { glView->setEditPixelMode(isClicked); }

        // SAMPLING tab

        // Camera range:
        void on_checkViewAzimuth_clicked(bool isClicked);
        void on_checkViewElevation_clicked(bool isClicked);
        void on_buttonAddView_clicked();
        void on_buttonRemoveView_clicked();
        void setRemoveViewStatus() { buttonRemoveView->setEnabled(!tableView->selectedItems().isEmpty()); }

        // View step size:
        void on_boxAngleY_valueChanged(double newValue) { imgSampler->setAngleY(newValue); }
        // void on_lineAngleX_textChanged(QString newText) { imgSampler->setAngleX(newValue); }
        // void on_lineDistance_textChanged(QString newText) { imgSampler->setDistance(newValue); }
		float findAngle(std::vector<float> intervals, float min, float max, float range, bool isCircular);

        // Image storage:
		void on_buttonModelFolder_clicked();
		void on_buttonOutputFolder_clicked();
		void on_buttonBackgroundFolder_clicked();
        void on_comboResSamples_currentIndexChanged(const QString & text) { imgSampler->makeCurrent(); imgSampler->updateSizeSample(extractNumberFromCombo(text)); glView->makeCurrent(); }
        void on_comboNumSamples_currentIndexChanged(const QString & text) { imgSampler->makeCurrent(); imgSampler->updateNumSamples(extractNumberFromCombo(text)); glView->makeCurrent(); }
        void updateSamplerInfo();
        static int extractNumberFromCombo(const QString & text)
        {
            QString pattern("x"); 
            QStringList chopped = text.split(pattern); 
            int checkValue = chopped[0].toInt();
            return checkValue;
        }
        void on_buttonPreview_clicked();
        void on_buttonSaveView_clicked();
        void cleanPreviewWidget() { delete imgSampler; }
        void on_buttonScript_clicked();
		void runScript();

		// KEYPOINT tab
		// - Kps list updates
		void on_listKps_itemSelectionChanged();
		void on_buttonAddKps_clicked();
		void on_buttonRemoveKps_clicked();
		void on_buttonUpKps_clicked();
		void on_buttonDownKps_clicked();
		void on_lineNewKps_editingFinished();
		// - Update of current part coordinate (X, Y, Z)
		void on_lineKpsX_editingFinished();
		void on_lineKpsY_editingFinished();
		void on_lineKpsZ_editingFinished();
		void on_buttonUpKpsX_clicked();
		void on_buttonDownKpsX_clicked();
		void on_buttonUpKpsY_clicked();
		void on_buttonDownKpsY_clicked();
		void on_buttonUpKpsZ_clicked();
		void on_buttonDownKpsZ_clicked();
		// - Update of current part scaling (Sx,Sy,Sz)
		void on_lineKpsSx_editingFinished();
		void on_lineKpsSy_editingFinished();
		void on_lineKpsSz_editingFinished();
		void on_buttonUpKpsSx_clicked();
		void on_buttonDownKpsSx_clicked();
		void on_buttonUpKpsSy_clicked();
		void on_buttonDownKpsSy_clicked();
		void on_buttonUpKpsSz_clicked();
		void on_buttonDownKpsSz_clicked();
		// - Activate special cases of objects
		void on_checkKpsNoAz_clicked();
		void on_checkKpsNoSelfOcc_clicked();
		// - File save with all parts' coordinates
		void on_buttonSaveKps_clicked();
		void updateKps(std::map<std::string, Kp> list_kps);

    private:

        DefaultParams defaultParams;
		std::string PATH_MODEL, PATH_BACKGROUND, PATH_OUTPUT, PATH_OBJ, PATH_INSTANCE, modelFileName;
        void getPaths();

        void embedGLWidget(QWidget* base, QWidget* glView);
        Render *glView;
        void updateViewerInfo();
        QWidget *winPreview, *winDepth;
        Sampler *imgSampler, *imgDepth;
        QLineEdit *lineNewName;

        static std::string floatToString(float aFloat)
        {
            std::ostringstream s;
            s << aFloat;
            return s.str();
        }
};

#endif
