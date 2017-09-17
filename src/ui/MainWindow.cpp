#include <iostream>
#include <time.h>

#include <QWidget>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QRect>
#include <QGLFormat>
#include <QTextStream>
#include <qDebug>

#include "ui/MainWindow.hpp"

using namespace std;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
	setupUi(this);

	// Get paths from config file
	getPaths();
	modelFileName = "";

	QGLFormat glFormat; //  (QGL::DoubleBuffer | QGL::DepthBuffer);
    glFormat.setVersion(4, 3);
    glFormat.setProfile(QGLFormat::CoreProfile); // Requires >=Qt-4.8.0
    glFormat.setSampleBuffers(false);
	glFormat.setSamples(64);
	glFormat.setDepth(true);
	glFormat.setDepthBufferSize(32);

	// Output image of samples preview
	winPreview = new QWidget();
	winPreview->setWindowTitle("Sample Preview");
	imgSampler = new Sampler(winPreview, glFormat);
	updateSamplerInfo();
	embedGLWidget(winPreview, imgSampler);
	winPreview->setFixedWidth(imgSampler->getSizeSample()*imgSampler->getNumSamples());
	winPreview->setFixedHeight(imgSampler->getSizeSample()*imgSampler->getNumSamples());
	winPreview->show();

	// Depth visualiser
	winDepth = new QWidget();
	winDepth->setWindowTitle("Depth Visualiser");
	imgDepth = new Sampler(winDepth, glFormat);
	int depthSize = 256;
	imgDepth->setSizeSample(depthSize); // standard 256.... but 512 for keypoint annotations
	imgDepth->setNumSamples(1);
	embedGLWidget(winDepth, imgDepth);
	winDepth->setFixedWidth(depthSize);
	winDepth->setFixedHeight(depthSize);
	winDepth->show();

	// Main render
	glView = new Render(this, glFormat, imgSampler, imgDepth, PATH_OUTPUT, PATH_OBJ);
	updateViewerInfo();
	embedGLWidget(frameRenderer, glView);

	// Set global positions in the screen (by now, hardcoded)
	QRect screenGeom = QDesktopWidget().availableGeometry();
	move((int)(screenGeom.width()*0.15), (int)(screenGeom.height()*0.15));
	winPreview->move(pos().x() + width() + 15, pos().y());
	winDepth->move((int)(screenGeom.width()*0.15) - imgDepth->getSizeSample() - 15, (int)(screenGeom.height()*0.15));

	// MainWindow <-> Renderer connections
	connect(glView, SIGNAL(updateFPS(const QString&)), showFPS, SLOT(setText(const QString&)));
	connect(glView, SIGNAL(updateBrushGUI(int)), spinBrushSize, SLOT(setValue(int)));
	connect(glView, SIGNAL(updateCompleteness()), this, SLOT(updateLabelCompleteness()));

	// View updates connections
	connect(tableView, SIGNAL(itemSelectionChanged()), this, SLOT(setRemoveViewStatus()));
	connect(tableView, SIGNAL(cellChanged(int, int)), this, SLOT(updateSamplerInfo()));

	// GUI internal actions
	lineNewName = new QLineEdit(QString("init"));
	connect(lineNewName, SIGNAL(editingFinished()), this, SLOT(endUpdateLabelName()));

	// Store default parameters and apply them
	defaultParams.objX = boxX->value();
	defaultParams.objY = boxY->value();
	defaultParams.objZ = boxZ->value();
	defaultParams.objRx = boxRX->value();
	defaultParams.objRy = boxRY->value();
	defaultParams.objRz = boxRZ->value();
	defaultParams.objSx = boxSX->value();
	defaultParams.objSy = boxSY->value();
	defaultParams.objSz = boxSZ->value();
	defaultParams.AR = checkAR->isChecked();
	defaultParams.azimuth = boxAngleY->value();
	defaultParams.elevation = lineAngleX->text();
	defaultParams.distance = lineDistance->text();
	defaultParams.AA = checkAA->isChecked();
	defaultParams.resSample = comboResSamples->currentIndex();
	defaultParams.numSamples = comboNumSamples->currentIndex();
	on_buttonImageDefault_clicked();

	glView->setFocus();

	on_checkKpsNoSelfOcc_clicked();
	on_checkKpsNoAz_clicked();

	on_tabWidget_currentChanged(0);
}

MainWindow::~MainWindow()
{
	delete winPreview, winDepth;
	delete imgSampler, imgDepth;
	delete glView;
	delete lineNewName;
}

void MainWindow::getPaths()
{
	QFile pathsFile;
	pathsFile.setFileName(":/config.txt");		
	if(!pathsFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Cannot open config file with paths!" << endl;
		exit(EXIT_FAILURE);
	}

	QTextStream in(&pathsFile);
    while (!in.atEnd())
	{
		QString strLine = in.readLine();

		QStringList strWords = strLine.split(" ");
		if(strWords.isEmpty()) // Empty line
			continue;
		else if(strWords[0].toStdString() == "#") // Comment
			continue;
		else if(strWords[0].toStdString() == "PATH_MODEL")
			PATH_MODEL = strWords[1].toStdString();
		else if(strWords[0].toStdString() == "PATH_BACKGROUND")
			PATH_BACKGROUND = strWords[1].toStdString();
		else if(strWords[0].toStdString() == "PATH_OUTPUT")
			PATH_OUTPUT = strWords[1].toStdString();
		else if(strWords[0].toStdString() == "PATH_OBJ")
			PATH_OBJ = strWords[1].toStdString();
	}
	pathsFile.close();
}

void MainWindow::embedGLWidget(QWidget* base, QWidget* glView)
{
	QHBoxLayout* layout = new QHBoxLayout();
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(glView);
	base->setLayout(layout);
}

void MainWindow::updateViewerInfo()
{
	float ambInt = 0.1f;
	ambInt = (float)sliderLighting1->value()/sliderLighting1->maximum()/Lights::MAX_LIGHTS; glView->getLights().diffuse[0] = glm::vec3(ambInt, ambInt, ambInt);
	ambInt = (float)sliderLighting2->value()/sliderLighting2->maximum()/Lights::MAX_LIGHTS; glView->getLights().diffuse[1] = glm::vec3(ambInt, ambInt, ambInt);
	ambInt = (float)sliderLighting3->value()/sliderLighting3->maximum()/Lights::MAX_LIGHTS; glView->getLights().diffuse[2] = glm::vec3(ambInt, ambInt, ambInt);
	ambInt = (float)sliderLighting4->value()/sliderLighting4->maximum()/Lights::MAX_LIGHTS; glView->getLights().diffuse[3] = glm::vec3(ambInt, ambInt, ambInt);
	ambInt = (float)sliderLighting5->value()/sliderLighting5->maximum()/Lights::MAX_LIGHTS; glView->getLights().diffuse[4] = glm::vec3(ambInt, ambInt, ambInt);
	ambInt = (float)sliderLighting6->value()/sliderLighting6->maximum()/Lights::MAX_LIGHTS; glView->getLights().diffuse[5] = glm::vec3(ambInt, ambInt, ambInt);
	ambInt = (float)sliderLighting7->value()/sliderLighting7->maximum()/Lights::MAX_LIGHTS; glView->getLights().diffuse[6] = glm::vec3(ambInt, ambInt, ambInt);
	ambInt = (float)sliderLighting8->value()/sliderLighting8->maximum()/Lights::MAX_LIGHTS; glView->getLights().diffuse[7] = glm::vec3(ambInt, ambInt, ambInt);
	on_sliderLight1X_valueChanged(); on_sliderLight1Y_valueChanged(); on_sliderLight1Z_valueChanged(); on_sliderLighting1_valueChanged(sliderLighting1->value());
	on_sliderLight2X_valueChanged(); on_sliderLight2Y_valueChanged(); on_sliderLight2Z_valueChanged(); on_sliderLighting2_valueChanged(sliderLighting2->value());
	on_sliderLight3X_valueChanged(); on_sliderLight3Y_valueChanged(); on_sliderLight3Z_valueChanged(); on_sliderLighting3_valueChanged(sliderLighting3->value());
	on_sliderLight4X_valueChanged(); on_sliderLight4Y_valueChanged(); on_sliderLight4Z_valueChanged(); on_sliderLighting4_valueChanged(sliderLighting4->value());
	on_sliderLight5X_valueChanged(); on_sliderLight5Y_valueChanged(); on_sliderLight5Z_valueChanged(); on_sliderLighting5_valueChanged(sliderLighting5->value());
	on_sliderLight6X_valueChanged(); on_sliderLight6Y_valueChanged(); on_sliderLight6Z_valueChanged(); on_sliderLighting6_valueChanged(sliderLighting6->value());
	on_sliderLight7X_valueChanged(); on_sliderLight7Y_valueChanged(); on_sliderLight7Z_valueChanged(); on_sliderLighting7_valueChanged(sliderLighting7->value());
	on_sliderLight8X_valueChanged(); on_sliderLight8Y_valueChanged(); on_sliderLight8Z_valueChanged(); on_sliderLighting8_valueChanged(sliderLighting8->value());
	glView->setAntiAliasing(checkAA->isChecked());
	glView->setViewBoundingBox(checkBB->isChecked());
	glView->setFreeCamera(boxCameraFree->isChecked());
	labelDistance->setText(QString("%1%2%3").arg("Distance (").arg(floatToString(sliderDistance->value()/10.0f).c_str()).arg("):"));
	glView->setFixedDistance(sliderDistance->value()/10.0f);
	labelElevation->setText(QString("%1%2%3").arg("Elevation (").arg(floatToString(sliderElevation->value()/10.0f).c_str()).arg("):"));
	glView->setFixedElevation(sliderElevation->value()/10.0f);
	labelAzimuth->setText(QString("%1%2%3").arg("Azimuth (").arg(floatToString(dialAzimuth->value()/10.0f).c_str()).arg("):"));
	glView->setFixedAzimuth(dialAzimuth->value()/10.0f);

	glView->setIsLabel(checkLabelling->isChecked());

	glView->setBrushSize(spinBrushSize->value());
	glView->setEditPixelMode(checkPixelMode->isChecked());
}

void MainWindow::updateSamplerInfo()
{
	imgSampler->setAngleY(boxAngleY->value());
	QStringList txtElev = lineAngleX->text().split(",");
	imgSampler->setAngleX(txtElev[0].toDouble());
	QStringList txtDist = lineDistance->text().split(",");
	imgSampler->setDistance(txtDist[0].toDouble());

	imgSampler->setSizeSample(extractNumberFromCombo(comboResSamples->currentText()));
	imgSampler->setNumSamples(extractNumberFromCombo(comboNumSamples->currentText()));

	// Copy views from GUI to Sampler
	imgSampler->resetViews();
	QString mText;
	int minA = 0; int maxA = 0; int minE = 0; int maxE = 0;
	for(int iView = 0; iView < tableView->rowCount(); ++iView)
	{
		if(tableView->item(iView, 0) != NULL)
			minA = tableView->item(iView, 0)->text().toInt();

		if(tableView->item(iView, 1) != NULL)
			maxA = tableView->item(iView, 1)->text().toInt();		
		
		if(tableView->item(iView, 2) != NULL)
			minE = tableView->item(iView, 2)->text().toInt();
		
		if(tableView->item(iView, 3) != NULL)
			maxE = tableView->item(iView, 3)->text().toInt();
		
		imgSampler->addView(minA, maxA, minE, maxE);
	}
}

void MainWindow::on_buttonModelFolder_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Select Model Folder"), PATH_MODEL.c_str(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	labelModel->setText(dir);
}

void MainWindow::on_buttonOutputFolder_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Folder"), PATH_OUTPUT.c_str(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	labelOutput->setText(dir);
}

void MainWindow::on_buttonBackgroundFolder_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Select Background Folder"), PATH_BACKGROUND.c_str(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	labelBackground->setText(dir);
}

void MainWindow::on_buttonLoadModel_clicked()
{
	QString qFileName = QFileDialog::getOpenFileName(this, "Open Model File", PATH_MODEL.c_str(),
		"(*.*);;3D Studio Max(*.3ds);;Wavefront OBJ (*.obj);;Collada (*.dae);;FilmBox (*.fbx);;DirectX (*.x);;");
	modelFileName = qFileName.toLatin1().data();
	// modelFileName = "Z:/PhD/Data/Syn/Car/set01/obj0001/Car_01.obj"; // hardcoded for testing purposes
	// modelFileName = "Z:/PhD/Data/Syn/Aeroplane/set01/obj0001/A-330.obj"; // hardcoded for testing purposes
	if (modelFileName != "")
	{
		cout << "Load model with path: " << modelFileName << endl;
		glView->loadModelFromFile(modelFileName, glView->getShader(TYPE_SHADER::PHONG));
		// Update Keypoints GUI + Render
		updateKps(glView->getModel()->getKps());
	}
}

void MainWindow::on_buttonImageDefault_clicked()
{
	boxX->setValue(defaultParams.objX);
	boxY->setValue(defaultParams.objY);
	boxZ->setValue(defaultParams.objZ);
	boxRX->setValue(defaultParams.objRx);
	boxRY->setValue(defaultParams.objRy);
	boxRZ->setValue(defaultParams.objRz);
	boxSX->setValue(defaultParams.objSx);
	boxSY->setValue(defaultParams.objSy);
	boxSZ->setValue(defaultParams.objSz);
	checkAR->setChecked(defaultParams.AR);

	boxAngleY->setValue(defaultParams.azimuth);
	lineAngleX->setText(defaultParams.elevation);
	lineDistance->setText(defaultParams.distance);
	
	comboResSamples->setCurrentIndex(defaultParams.resSample);
	comboNumSamples->setCurrentIndex(defaultParams.numSamples);
	checkAA->setChecked(defaultParams.AA);
}

void MainWindow::on_buttonLoadBackground_clicked()
{
	QString qFileName = QFileDialog::getOpenFileName(this, "Open Model File", PATH_BACKGROUND.c_str(),
		"PNG (*.png);; JPEG (*.jpg);; BMP (*.bmp);;");
	string fileName = qFileName.toLatin1().data();
	if(fileName != "")
	{
		cout << "Load background image with path:\n " << fileName << endl;
		glView->loadBackgroundImgFromFile(fileName);
		QPixmap pixmap(fileName.c_str());
		QIcon ButtonIcon(pixmap);
		imgBackground->setIcon(ButtonIcon);
	}

	buttonResetBackground->setEnabled(true);
}

void MainWindow::on_buttonResetBackground_clicked()
{
	cout << "Reset background image" << endl;
	glView->loadBackgroundImgFromFile("");
	imgBackground->setIcon(QIcon());
	buttonResetBackground->setEnabled(false);
}

vector<unsigned int> MainWindow::getPathNode(QTreeWidgetItem* item)
{
	// Track tree path from node to root
	vector<unsigned int> pathNode;

	if(item->parent())
	{
		QTreeWidgetItem* parent = item->parent();
		QTreeWidgetItem* child = item;
		while(parent)
		{
			pathNode.push_back(parent->indexOfChild(child));
			child = parent;
			parent = parent->parent();
		}
	}

	reverse(pathNode.begin(), pathNode.end());
	return pathNode;
}

void MainWindow::on_buttonNewLabel_clicked()
{
	QTreeWidgetItem* parent = treeLabelling->currentItem();
	QTreeWidgetItem* child = new QTreeWidgetItem();
	
	parent->addChild(child);
	parent->setExpanded(true);

	vector<unsigned int> pathNode = getPathNode(parent);
	child->setBackgroundColor(0, QColor(rand()%205+50, rand()%205+50, rand()%205+50));
	child->setText(0, QString("%1%2%3%4").arg("L").arg(pathNode.size() + 1).arg(" P").arg(parent->childCount()));

	if(glView->isModel())
	{
		glView->getModel()->addTreePart(pathNode);
		pathNode.push_back(parent->childCount()-1);
		glView->getModel()->setColourTreePart(pathNode, child->backgroundColor(0).redF(), child->backgroundColor(0).greenF(), child->backgroundColor(0).blueF());
	}

	updateLabelCompleteness();
}

void MainWindow::on_buttonDeleteLabel_clicked()
{
	QTreeWidgetItem* parent = treeLabelling->currentItem()->parent();

	if(parent != NULL)
		parent->removeChild(treeLabelling->currentItem());
	
	vector<unsigned int> pathNode = getPathNode(treeLabelling->currentItem());

	if(glView->isModel())
		glView->getModel()->removeTreePart(pathNode);

	updateLabelCompleteness();

	// Select previous child or parent (if none) as current new label
	if(parent->childCount() == 0)
	{
		treeLabelling->setCurrentItem(parent);
		on_treeLabelling_itemClicked(parent, 0);
	}
	else
	{
		treeLabelling->setCurrentItem(parent->child(parent->childCount() - 1));
		on_treeLabelling_itemClicked(parent->child(parent->childCount() - 1), 0);
	}
}

void MainWindow::on_treeLabelling_itemClicked(QTreeWidgetItem * item, int column)
{
	vector<unsigned int> pathNode = getPathNode(item);

	// Update GUI
	labelLevel->setText(QString("Level %1").arg(pathNode.size()));

	// Enable/Disable add/remove buttons
	buttonNewLabel->setEnabled(true);
	
	if(item->parent() != NULL)
		buttonDeleteLabel->setEnabled(true);
	else
		buttonDeleteLabel->setEnabled(false);

	// Assign as current highlighted entity
	if(glView->isModel())
		glView->getModel()->setCurrentTreeNode(pathNode);

	// Check level completeness to update GUI
	updateLabelCompleteness();
}

void MainWindow::updateLabelCompleteness()
{
	if(glView->getModel()->checkLevelCompleteness())
		labelLevelCompleteness->setText("YES");
	else
		labelLevelCompleteness->setText("NO");

	labelNumCompleteness->setText(QString("%1/%2").arg(glView->getModel()->getFacesChildren()).arg(glView->getModel()->getFacesParent()));

	if(glView->getModel()->checkTreeCompleteness())
	{
		labelTreeCompleteness->setText("YES");
		buttonSaveLabelling->setEnabled(true);
	}
	else
	{
		labelTreeCompleteness->setText("NO");
		buttonSaveLabelling->setEnabled(false);
	}
}

void MainWindow::on_treeLabelling_itemDoubleClicked(QTreeWidgetItem* item, int column)
{
	const QColor colour = QColorDialog::getColor();
    if (colour.isValid())
		item->setBackgroundColor(0, colour);

	// Update part colour
	int level, label = 0;
	if(item->parent()) // It is not the root level
	{
		level = item->parent()->statusTip(0).toInt() + 1;
		label = item->parent()->indexOfChild(item);
	}
	
	vector<unsigned int> pathNode = getPathNode(item);
	if(glView->isModel())
		glView->getModel()->setColourTreePart(pathNode, colour.redF(), colour.greenF(), colour.blueF());
}

void MainWindow::on_buttonSaveLabelling_clicked()
{
	QString segPath = QString(PATH_INSTANCE.c_str());
	segPath.chop(segPath.length() - segPath.lastIndexOf(QString(".")));
	segPath.append(".seg");

	string message = "Current segmentation will be stored at the following path:\n";
	message.append(segPath.toStdString());
	QMessageBox::StandardButton reply = QMessageBox::information(this, "Attention!", message.c_str(), QMessageBox::Ok);

	// cout << "SAVE SEGMENTATION at: " << fileName.c_str() << endl;
	ofstream segmentationFile(segPath.toStdString());
	if(segmentationFile.is_open())
	{
		segmentationFile << "#Segmentation of object at " << segPath.toStdString() << endl;
		QTreeWidgetItem* root = treeLabelling->topLevelItem(0);
		saveNodeLabelling(segmentationFile, root);
	}
	segmentationFile.close();

	message = "Segmentation succesfully stored!";
	reply = QMessageBox::information(this, "Attention!", message.c_str(), QMessageBox::Ok);
}

void MainWindow::saveNodeLabelling(ofstream& segmentationFile, QTreeWidgetItem* node)
{
	// Add current node's labelling
	segmentationFile  << "# NEW LABEL:" << endl;

	// Store name
	segmentationFile << "k " << node->text(0).toStdString().c_str() << endl;

	// Store colour
	QColor rgb = node->backgroundColor(0);
	segmentationFile << "c " << rgb.red() << " " << rgb.green() << " " << rgb.blue() << endl;

	// Store tree path
	segmentationFile << "p";
	vector<unsigned int> treePath = getPathNode(node);
	for(unsigned i = 0; i < treePath.size(); ++i)
		segmentationFile << " " << treePath[i];
	segmentationFile << endl;
	

	// Store 3D Information (Vertices and Faces)
	glView->saveSegmentationToFile(segmentationFile, getPathNode(node));
	
	// Space between label parts
	segmentationFile << endl;

	for(int idxChild = 0; idxChild < node->childCount(); ++idxChild)
		saveNodeLabelling(segmentationFile, node->child(idxChild));	
}

void MainWindow::on_buttonLoadLabelling_clicked()
{
	string message = "Would you like to load the stored object segmention, if any?\n (current segmentation will be overwritten!)";
	QMessageBox::StandardButton reply = QMessageBox::warning(this, "Attention!", message.c_str(), QMessageBox::Ok|QMessageBox::Cancel);
	if (reply == QMessageBox::Ok)
	{
		QString segPath = QString(PATH_INSTANCE.c_str());
		segPath.chop(segPath.length() - segPath.lastIndexOf(QString(".")));
		segPath.append(".seg");
		cout << "Load semantic segmentation with path: " << segPath.toStdString() << endl;

		string message = "Segmentation succesfully loaded!";
		ifstream segmentationFile(segPath.toStdString());
		if (segmentationFile.is_open())
		{
			string line, labelName;
			QString delimiters(" ");
			QStringList labelNodes, rgb;
			QTreeWidgetItem* newItem;
			QColor color;
			vector<unsigned int> treePath;
			while(getline(segmentationFile, line))
			{
				if(!line.empty())
				{			
					// cout << line << endl;
					switch(line[0])
					{
						case '#': // Ignore comment lines
							break;
						case 'k': // Name of the label part
							labelName = line.erase(0,2);
							break;
						case 'c': // Colour of the label part
							rgb = QString(line.erase(0,2).c_str()).split(delimiters);
							break;
						case 'p': // Tree position of the label part
							if(!line.erase(0,2).empty())
								labelNodes = QString(line.c_str()).split(delimiters);
							else
								labelNodes = QStringList();
							treePath.clear();
							treePath.resize(labelNodes.size());
							newItem = new QTreeWidgetItem();
							color = QColor(atoi(rgb[0].toStdString().c_str()), atoi(rgb[1].toStdString().c_str()), atoi(rgb[2].toStdString().c_str()));
							newItem->setBackgroundColor(0, color);
							newItem->setText(0, labelName.c_str());
							if(treePath.empty())
							{
								treeLabelling->clear();
								treeLabelling->addTopLevelItem(newItem);
							}
							else
							{
								QTreeWidgetItem* item = treeLabelling->topLevelItem(0);
								for(int i = 0; i < labelNodes.size(); ++i)
								{
									treePath[i] = atoi(labelNodes[i].toStdString().c_str());
									if(labelNodes.size()-1 != i)
										item = item->child(treePath[i]);
									else
									{
										item->addChild(newItem);
										item->setExpanded(true);
									}
								}
							}

							// Vertices, Normals and faces
							glView->loadSegmentationFromFile(segmentationFile, treePath, color.red()/255.0f, color.green()/255.0f, color.blue()/255.0f);
							break;
						default: 
							break;

					}
				}				
			}
			segmentationFile.close();
		}
		else
			message = "No segmentation file could not be read!";
		QMessageBox::StandardButton reply = QMessageBox::information(this, "Attention!", message.c_str(), QMessageBox::Ok);
	}
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
	bool isEditMode = (index == 1); // Semantics tab
	glView->setIsEditMode(isEditMode);
	boxCameraFree->setEnabled(!isEditMode);
	if (isEditMode)
		glView->setFreeCamera(false);

	isEditMode = (index == 3); // Keypoints tab
	glView->setIsKpsMode(isEditMode);
}

void MainWindow::updateLabelName(const QString& newLabel)
{
	QString copyLabel = QString(newLabel);
	treeLabelling->currentItem()->setText(0, copyLabel);
}
void MainWindow::endUpdateLabelName()
{
	// std::cout << "Finish rename mode!" << std::endl;
	lineNewName->hide();
	disconnect(this, SLOT(updateLabelName(const QString&)));
}

void MainWindow::on_checkLabelling_clicked(bool isClicked)
{
	glView->setIsLabel(isClicked);
}

void MainWindow::on_buttonAutomaticLabelling_clicked()
{
	string message = "Do you want to compute the labelling automatically?\n (previous manual selection will be overwritten!)";
	QMessageBox::StandardButton reply = QMessageBox::warning(this, "Attention!", message.c_str(), QMessageBox::Ok|QMessageBox::Cancel);
	if (reply == QMessageBox::Ok)
		cout << "lalalalala" << endl;
}

void MainWindow::on_checkViewAzimuth_clicked(bool isClicked)
{
	imgSampler->setIsAzimuth(isClicked);
	QBrush colour;
	QColor colourHeader;
	if(isClicked)
	{
		colour.setColor(QColor(0, 0, 0));
		colourHeader = QColor(0, 0, 0);
	}
	else
	{
		colour.setColor(QColor(200, 200, 200));
		colourHeader = QColor(200, 200, 200);
	}

	tableView->horizontalHeaderItem(0)->setTextColor(colourHeader);
	tableView->horizontalHeaderItem(1)->setTextColor(colourHeader);

	for(int iView = 0; iView < tableView->rowCount(); ++iView)
	{
		tableView->item(iView,0)->setForeground(colour);
		tableView->item(iView,1)->setForeground(colour);
	}
}

void MainWindow::on_checkViewElevation_clicked(bool isClicked)
{
	imgSampler->setIsElevation(isClicked);
	QBrush colour;
	QColor colourHeader;
	if(isClicked)
	{
		colour.setColor(QColor(0, 0, 0));
		colourHeader = QColor(0, 0, 0);
	}
	else
	{
		colour.setColor(QColor(200, 200, 200));
		colourHeader = QColor(200, 200, 200);
	}

	tableView->horizontalHeaderItem(2)->setTextColor(colourHeader);
	tableView->horizontalHeaderItem(3)->setTextColor(colourHeader);

	for(int iView = 0; iView < tableView->rowCount(); ++iView)
	{
		tableView->item(iView,2)->setForeground(colour);
		tableView->item(iView,3)->setForeground(colour);
	}
}

void MainWindow::on_buttonAddView_clicked()
{
	// Adds a default row at the back of the list
	tableView->insertRow(tableView->rowCount());

	QTableWidgetItem* item0 = new QTableWidgetItem(QString("0"));
	item0->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	tableView->setItem(tableView->rowCount()-1, 0, item0);
	QTableWidgetItem* item1 = new QTableWidgetItem(QString("360"));
	item1->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	tableView->setItem(tableView->rowCount()-1, 1, item1);
	QTableWidgetItem* item2 = new QTableWidgetItem(QString("0"));
	item2->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	tableView->setItem(tableView->rowCount()-1, 2, item2);
	QTableWidgetItem* item3 = new QTableWidgetItem(QString("90"));
	item3->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	tableView->setItem(tableView->rowCount()-1, 3, item3);
}

void MainWindow::on_buttonRemoveView_clicked()
{
	// A row is removed if a line is selected
	tableView->removeRow(tableView->currentRow());
	updateSamplerInfo();
}

void MainWindow::on_buttonPreview_clicked()
{
	bool isPreviewed = glView->previewSamples();
	if(isPreviewed)
		cout << "preview successfully done!" << endl;
	else
		cout << "preview NOT possible!" << endl;
}

void MainWindow::on_buttonSaveView_clicked()
{
	imgSampler->setNumImg(1);
	bool isSaved = glView->saveViewToImage();
	if(isSaved)
		cout << "image saved: OK!" << endl;
	else
		cout << "image saved: NO!" << endl;
}

void MainWindow::on_buttonScript_clicked()
{
	// Hardcoded info for generating synthetic data (ObjectNet3D)
	/*
	const string classes[] = { "Aeroplane", "Bicycle", "Boat", "Bottle", "Bus", "Car", "Chair", "Diningtable", "Motorbike", "Sofa", "Train", "TVMonitor" };
	labelBackground->setText(QString("Z:/PhD/Data/Real/Car/KITTI/test/images"));
	// For random stuff
	lineRandomMaxSamples->setText("1000");
	radioInterval->setChecked(true);
	radioRange->setChecked(false);
	vector<string> textAz, textEl, textTh, textDist;
	// -> Aeroplane
	textAz.push_back("0,15,30,45,60,75,90,105,120,135,150,165,180,195,210,225,240,255,270,285,300,315,330,345");
	textEl.push_back("-60,-45,-30,-15,0,15,30,75");
	textTh.push_back("-30,-15,0,15,30,45");
	textDist.push_back("1.2,2.5");
	// - Bicycle
	textAz.push_back("0,15,30,45,60,75,90,105,120,135,150,165,180,195,210,225,240,255,270,285,300,315,330,345");
	textEl.push_back("-30,-15,0,15,30,45");
	textTh.push_back("-30,-15,0,15,30");
	textDist.push_back("1.2,2.5");
	// - Boat
	textAz.push_back("0,15,30,45,60,75,90,105,120,135,150,165,180,195,210,225,240,255,270,285,300,315,330,345");
	textEl.push_back("-15,0,15,30");
	textTh.push_back("-15,0,15");
	textDist.push_back("1.2,2.5");
	// - Bottle
	textAz.push_back("0,15,30,345");
	textEl.push_back("-45,-30,-15,0,15,30,45,60");
	textTh.push_back("-180,-45,-15,0,15,30");
	textDist.push_back("1.2,2.5");
	// - Bus
	textAz.push_back("0,15,30,45,60,75,90,105,120,135,150,165,180,195,210,225,240,255,270,285,300,315,330,345");
	textEl.push_back("-15,0,15");
	textTh.push_back("-15,0,15");
	textDist.push_back("1.2,2.5");
	// - Car
	textAz.push_back("0,15,30,45,60,75,90,105,120,135,150,165,180,195,210,225,240,255,270,285,300,315,330,345");
	textEl.push_back("-15,0,15,30");
	textTh.push_back("-15,0,15");
	textDist.push_back("1.2,2.5");
	// - Chair
	textAz.push_back("0,15,30,45,60,75,90,150,165,270,285,300,315,330,345");
	textEl.push_back("-15,0,15,30,45");
	textTh.push_back("-15,0,15");
	textDist.push_back("1.2,2.5");
	// - Diningtable
	textAz.push_back("0,15,30,45,60,75,90,270,285,300,315,330,345");
	textEl.push_back("0,15,30,45");
	textTh.push_back("-15,0,15");
	textDist.push_back("1.2,2.5");
	// - Motorbike
	textAz.push_back("0,15,30,45,60,75,90,105,120,135,150,165,180,195,210,225,240,255,270,285,300,315,330,345");
	textEl.push_back("-30,-15,0,15,30");
	textTh.push_back("-30,-15,0,15,30");
	textDist.push_back("1.2,2.5");
	// - Sofa
	textAz.push_back("0,15,30,45,60,300,315,330,345");
	textEl.push_back("-15,0,15,30");
	textTh.push_back("-15,0,15,30");
	textDist.push_back("1.2,2.5");
	// - Train
	textAz.push_back("0,15,30,45,315,330,345");
	textEl.push_back("-15,0,15,30");
	textTh.push_back("-30,-15,0,15,30");
	textDist.push_back("1.2,2.5");
	// - TVMonitor
	textAz.push_back("0,15,30,45,60,300,315,330,345");
	textEl.push_back("-30,-15,0,15,30,45");
	textTh.push_back("-15,0,15");
	textDist.push_back("1.2,2.5");
	*/
	/*
	checkRandom->setChecked(false);
	vector<string> textEl, textTh, textDist;
	textTh.push_back("0");
	textDist.push_back("2.0");
	// -> Aeroplane
	textEl.push_back("-30, 0, 25, 50");
	// - Bicycle
	textEl.push_back("-20, 0, 20, 40");
	// - Boat
	textEl.push_back("-15, 0, 15, 30");
	// - Bottle
	textEl.push_back("-25, 0, 20, 40");
	// - Bus
	textEl.push_back("-10, 0, 15, 30");
	// - Car
	textEl.push_back("-10, 0, 15, 30");
	// - Chair	
	textEl.push_back("-10, 5, 20, 35");
	// - Diningtable
	textEl.push_back("0, 15, 30, 45");
	// - Motorbike
	textEl.push_back("-15, 0, 15, 30");
	// - Sofa
	textEl.push_back("-10, 0, 15, 30");
	// - Train
	textEl.push_back("-10, 0, 15, 30");
	// - TVMonitor
	textEl.push_back("-15, 0, 15, 30");
	*/ 

	/*
	// For 360 az
	lineTilt->setText(QString(textTh[0].c_str()));
	lineDistance->setText(QString(textDist[0].c_str()));
	for (unsigned int i = 0; i < 11; ++i)
	{
		if (i == 3) // bottle (+ rounded table case in script code)
		{
			checkKpsNoAz->setChecked(true);
			checkKpsNoSelfOcc->setChecked(true);
		}
		else
		{
			checkKpsNoAz->setChecked(false);
			checkKpsNoSelfOcc->setChecked(false);
		}
		on_checkKpsNoSelfOcc_clicked();
		on_checkKpsNoAz_clicked();
		
		labelModel->setText(QString(string("Z:/PhD/Data/Syn/" + classes[i] + "/set01").c_str()));
		labelOutput->setText(QString(string("Z:/PhD/Data/Syn/" +  classes[i] + "/p3d_1000").c_str()));
		// For 360 az (Journal DA)
		// lineAngleX->setText(QString(textEl[i].c_str()));

		// For random stuff (Paper Kps+Vp)
		lineAngleY->setText(QString(textAz[i].c_str()));
		lineAngleX->setText(QString(textEl[i].c_str()));
		lineTilt->setText(QString(textTh[i].c_str()));
		lineDistance->setText(QString(textDist[i].c_str()));

		runScript();
	}
	exit(0);
	*/

	runScript();
	exit(0);
}

void MainWindow::runScript()
{
	srand(1);
	clock_t timeScript = clock();

	if (!labelModel->text().compare(QString("-")) || !labelOutput->text().compare(QString("-")) || !labelBackground->text().compare(QString("-")))
	{
		cout << "Folders for model selection, output and background image not selected" << endl;
		return;
	}

	// Setup config
	imgSampler->makeCurrent();
	glView->makeCurrent();

	// Do not see the whole GUI in non-random generation
	// Note: for random generation stay shown to correctly update window sizes
	/*
	if (!checkRandom->isChecked())
	{
		hide();
		winPreview->hide();
		winDepth->hide();
	}
	*/

	// Azimuth interval
	boxAngleY->setValue(boxAngleY->value());
	// Range
	bool isRange = radioRange->isChecked();
	float step = boxAngleDisc->value();
	// Elevation and distance discrete values
	vector<float> azimuths, elevations, tilts, distances;
	if (!isRange)
	{
		QStringList txtAz = lineAngleY->text().split(",");
		for (int i = 0; i < txtAz.size(); ++i)
			azimuths.push_back(txtAz[i].toDouble());
	}
	QStringList txtElev = lineAngleX->text().split(",");
	for (int i = 0; i < txtElev.size(); ++i)
		elevations.push_back(txtElev[i].toDouble());
	QStringList txtTilt = lineTilt->text().split(",");
	for (int i = 0; i < txtTilt.size(); ++i)
		tilts.push_back(txtTilt[i].toDouble());
	QStringList txtDist = lineDistance->text().split(",");
	for (int i = 0; i < txtDist.size(); ++i)
		distances.push_back(txtDist[i].toDouble());

	if (elevations.empty() || distances.empty()|| tilts.empty() || (!isRange && azimuths.empty()))
	{
		cout << "Wrong elevation, distance or tilt selection\n" << endl;
		return;
	}

	// Filter of 3D model extensions
	QStringList extFiles;
	string ext = string("*").append(lineExt->text().toStdString());
	extFiles << ext.c_str();

	// Create save directory if does not exist
	string savePath = labelOutput->text().toStdString() + "/";
	QDir saveDir(savePath.c_str());
	if (!saveDir.exists())
		QDir().mkdir(savePath.c_str());

	// Create a README.txt file to describe annotations and names
	ofstream readmeFile;
	readmeFile.open(savePath + "README.txt");
	if(readmeFile.is_open())
	{
		readmeFile << ">> README.txt" << endl;
		readmeFile << "FOLDER STRUCTURE: " << endl;
		readmeFile << "img_pNUM_INSTANCE" << endl;
		readmeFile << "ANNOTATION STRUCTURE: " << endl;
		readmeFile << "IMG_NAME ROW COL HEIGHT WIDTH AZIMUTH ELEVATION DISTANCE PART_NAME_1 PART_1_X PART_1_Y PART_1_Z" << endl;
	}
	readmeFile.close();

	cout << endl << "Generation of synthetic images" << endl;
	string pathSet;
	int idxNewModel = 0;
	for(int iSet = 0; iSet < 1 /*listSets.size()*/; ++iSet)
	{
		QDir dirModels(labelModel->text());
		dirModels.setFilter(QDir::Dirs| QDir::NoDotAndDotDot);
		QStringList listModels = dirModels.entryList();

		string pathModel;
		for (int iModel = 0; iModel < listModels.size(); ++iModel)
		{
			// Hardcoded for rounded tables:
			if (iModel > 7 && QString(savePath.c_str()).contains("Diningtable"))
			{
				tilts.clear(); tilts.push_back(0);
				step = 1.0f;
				checkKpsNoAz->setChecked(true);
				on_checkKpsNoAz_clicked();				
			}

			imgSampler->setNumImg(1);
			// Find files that can be read (.obj, .3ds, .dae)
			pathModel = labelModel->text().toStdString() + "/";
			pathModel.append(listModels[iModel].toStdString());
			QDir dirFiles(pathModel.c_str());
			dirFiles.setFilter(QDir::Files| QDir::NoDotAndDotDot);
			dirFiles.setNameFilters(extFiles);
			QStringList listFiles = dirFiles.entryList();

			// No 3D model found... try next folder...
			if(listFiles.empty())
				continue;

			cout << endl << "Model " << idxNewModel+1 << ": " << endl;
			pathModel.append("/" + listFiles[0].toStdString());

			string saveObj = savePath;
			saveObj.append("/obj_");
			saveObj.append(imgSampler->IntToStr(idxNewModel));

			// Loading model... ... ...
			bool isLoaded = glView->loadModelFromFile(pathModel, glView->getShader(TYPE_SHADER::PHONG));
			// Update Keypoints GUI + Render
			updateKps(glView->getModel()->getKps());

			if (checkRandom->isChecked()) // Random sampling
			{
				int azStep = (int)boxAngleY->value();
				int azRange = (int)floor(360.0 / boxAngleY->value());
				boxAngleY->setValue(360.0); // Only one pass (1 image per random azimuth)
				unsigned int numSamplesModel = lineRandomMaxSamples->text().toInt();
				if (numSamplesModel == 0)
				{
					cout << "Wrong input for number of random samples per model" << endl;
					return;
				}
				if (isRange && (elevations.size() != 2 || distances.size() != 2 || tilts.size() != 2))
				{
					cout << "Wrong input for elevation/distance ranges (2 elems: lower and upper bound)" << endl;
					return;
				}
				for (unsigned int idxSample = 0; idxSample < numSamplesModel; ++idxSample)
				{
					// Change image size
					imgSampler->makeCurrent();
					imgSampler->updateNumSamples(1);
					double r = ((double)rand() / RAND_MAX) * 0.5 + 0.75;
					int sizeSample = (int)floor(512.0 * r);
					imgSampler->updateSizeSample(sizeSample);
					imgSampler->setSizeSample(sizeSample);
					glView->makeCurrent();
					// Select random az, el, th, d
					// - Set random azimuth based on granularity
					float az = 0.0f;
					if (isRange)
						az = (rand() % azRange) * azStep;
					else
						az = findAngle(azimuths, 0, 360, step, true);
					imgSampler->setCurrentAngleY(az);
					// - Set random elevation based on range input (2 elems)
					float el = 0.0f;
					if (isRange)
						el = elevations[0] + ((double)rand() / RAND_MAX)*(elevations[1] - elevations[0]);
					else
						el = findAngle(elevations, -90, 90, step, false);
					imgSampler->setAngleX(floor(el * 100)/100.0);					
					// - Set random tilt based on range input (2 elems)
					float th = 0.0f;
					if (isRange)
						th = tilts[0] + ((double)rand() / RAND_MAX)*(tilts[1] - tilts[0]);
					else
						th = findAngle(tilts, -180, 180, step, true);
					imgSampler->setTilt(floor(th * 100) / 100.0);
					// - Set random distance based on range input (2 elems) -> always range!
					float d = distances[0] + ((double)rand() / RAND_MAX)*(distances[1] - distances[0]);
					imgSampler->setDistance(floor(d * 100) / 100.0);
					// Bg image
					string pathBackground = labelBackground->text().toStdString();
					pathBackground.append("/");
					int idxBackground = rand() % 7517 + 1;
					string strNum = imgSampler->IntToStr(idxBackground);
					for (unsigned i = 0; i < 6 - strNum.length(); ++i)
						pathBackground.append("0");
					pathBackground.append(strNum);
					pathBackground.append(".png");
					glView->loadBackgroundImgFromFile(pathBackground);
					// Update lights randomly within a specific range (+ intensity)
					float light_range = 0.5;
					vector<glm::vec3> old_lights(8);
					for (int l = 0; l < Lights::MAX_LIGHTS; ++l)
					{
						old_lights[l] = glView->getLights().position[l];
						float light = ((float)rand() / RAND_MAX)*light_range - light_range / 2.0f;
						glView->getLights().position[l] += glm::vec3(light, light, light);
						light = ((float)rand() / RAND_MAX) * 0.5 + 0.25;
						glView->getLights().diffuse[l] = glm::vec3(light, light, light);
					}

					// Store random image
					// update objet scaling randomly within a specific range
					float maxScale = 1.25f; float minScale = 0.75;
					// Hardcoded for rounded tables:
					if (iModel > 7 && QString(savePath.c_str()).contains("Diningtable"))
					{
						maxScale = 1.0f;
						minScale = 1.0f;
					}
					float scaleX = (float)rand() / RAND_MAX * (maxScale - minScale) + minScale;
					float scaleY = (float)rand() / RAND_MAX * (maxScale - minScale) + minScale;
					float scaleZ = (float)rand() / RAND_MAX * (maxScale - minScale) + minScale;
					glView->getModel()->setScaling(scaleX, scaleY, scaleZ);
					glView->saveViewToImage(saveObj);

					// Restore light position
					for (int l = 0; l < Lights::MAX_LIGHTS; ++l)
						glView->getLights().position[l] = old_lights[l];
				}
				boxAngleY->setValue(azStep);
			}
			else
			{
				for (unsigned int idxTilt = 0; idxTilt < tilts.size() && isLoaded; ++idxTilt)
				{
					// Update tilt
					imgSampler->setTilt(tilts[idxTilt]);

					for (unsigned int idxElevation = 0; idxElevation < elevations.size() && isLoaded; ++idxElevation)
					{
						// Update elevation
						imgSampler->setAngleX(elevations[idxElevation]);

						for (unsigned int idxDistance = 0; idxDistance < distances.size() && isLoaded; ++idxDistance)
						{
							imgSampler->setDistance(distances[idxDistance]);

							// Load random background
							string pathBackground = labelBackground->text().toStdString();
							pathBackground.append("/");

							// int idxBackground = rand() % 711 + 1;
							// pathBackground.append("/neg (");
							// pathBackground.append(imgSampler->IntToStr(idxBackground));
							// pathBackground.append(").png");
							int idxBackground = rand() % 7517 + 1;
							string strNum = imgSampler->IntToStr(idxBackground);
							for (unsigned i = 0; i < 6 - strNum.length(); ++i)
								pathBackground.append("0");
							pathBackground.append(strNum);
							pathBackground.append(".png");


							// White background for testing
							// pathBackground = "D:/PhD/Data/Syn/whiteBg.png";
							glView->loadBackgroundImgFromFile(pathBackground);

							// Start from azimuth angle  0
							imgSampler->resetCurrentAngleY();
							// Once all setup is done, store the images
							glView->saveViewToImage(saveObj);
						}
					}
				}
			}
			idxNewModel++;
		}
	}

	// Show again GUI
	show();
	glView->show();
	winPreview->show();
	winDepth->show();

	timeScript = (float)clock() - (float)timeScript;
	cout << "Time script: " << timeScript << "ms" << endl << endl;
}

float MainWindow::findAngle(vector<float> intervals, float minValue, float maxValue, float step, bool isCircular)
{
	while (1)
	{
		float value = minValue + ((float)rand() / RAND_MAX) * (maxValue - minValue);
		float minDist = 9999.0f;
		float minValue = 0.0f;
		for (unsigned int i = 0; i < intervals.size(); ++i)
		{
			if (isCircular)
			{
				float dist = min(abs(intervals[i] - value), abs(intervals[i] - 360 - value));
				if (dist < minDist)
					minDist = dist;
			}
			else
			{
				float dist = abs(intervals[i] - value);
				if (dist < minDist)
					minDist = dist;
			}

			if (minDist < step / 2.0f)
				return value;
		}
	}
}

// KEYPOINT tab
// - Kps list updates
void MainWindow::on_listKps_itemSelectionChanged()
{
	QListWidgetItem * item = listKps->currentItem();
	string id = item->text().toStdString();
	// Update visualisation
	glView->drawActiveKps(id);
	glView->getModel()->setKpsActive(id);
	// Update XYZ coordinates in GUI
	Kp kp = glView->getModel()->getKpsId(id);
	lineKpsX->setText(QString::number(kp.X));
	lineKpsY->setText(QString::number(kp.Y));
	lineKpsZ->setText(QString::number(kp.Z));
	lineKpsSx->setText(QString::number(kp.Sx));
	lineKpsSy->setText(QString::number(kp.Sy));
	lineKpsSz->setText(QString::number(kp.Sz));
}

void MainWindow::on_buttonAddKps_clicked()
{
	QString name = lineNewKps->text();
	float X = lineKpsX->text().toFloat();
	float Y = lineKpsY->text().toFloat();
	float Z = lineKpsZ->text().toFloat();
	float Sx = lineKpsSx->text().toFloat();
	float Sy = lineKpsSy->text().toFloat();
	float Sz = lineKpsSz->text().toFloat();
	if (!name.isEmpty())
	{
		listKps->addItem(name);

		glView->getModel()->addKps(name.toStdString(), X, Y, Z, Sx, Sy, Sz, listKps->count()-1);
		glView->createKp(glView->getModel()->getKpsId(name.toStdString()));
	}
}
void MainWindow::on_buttonRemoveKps_clicked()
{
	int currentIndex = listKps->currentRow();
	if (currentIndex != -1)
	{
		string name;
		if (listKps->count() > 1)
		{
			QListWidgetItem* item = listKps->takeItem(currentIndex);
			name = item->text().toStdString();
			delete item;
			for (unsigned int i = currentIndex; i < listKps->count(); ++i)
				glView->getModel()->updateKpsPos(listKps->item(currentIndex)->text().toStdString(), currentIndex);
		}
		else
		{
			name = listKps->selectedItems().first()->text().toStdString();
			listKps->clear();
			lineKpsX->setText(QString(""));
			lineKpsY->setText(QString(""));
			lineKpsZ->setText(QString(""));
			lineKpsSx->setText(QString(""));
			lineKpsSy->setText(QString(""));
			lineKpsSz->setText(QString(""));
		}
		glView->getModel()->removeKps(name);
		glView->destroyKp(name);
	}
}
void MainWindow::on_buttonUpKps_clicked()
{
	int currentIndex = listKps->currentRow();
	if (currentIndex != -1)
	{
		QListWidgetItem* currentItem = listKps->takeItem(currentIndex);
		if (currentIndex > 0)
		{
			listKps->insertItem(currentIndex - 1, currentItem);
			listKps->setCurrentRow(currentIndex - 1);
			glView->getModel()->updateKpsPos(currentItem->text().toStdString(), currentIndex - 1);
			glView->getModel()->updateKpsPos(listKps->item(currentIndex)->text().toStdString(), currentIndex);
		}
	}
}
void MainWindow::on_buttonDownKps_clicked()
{
	int currentIndex = listKps->currentRow();
	if (currentIndex != listKps->count() - 1)
	{
		QListWidgetItem* currentItem = listKps->takeItem(currentIndex);
		listKps->insertItem(currentIndex + 1, currentItem);
		listKps->setCurrentRow(currentIndex + 1);
		glView->getModel()->updateKpsPos(currentItem->text().toStdString(), currentIndex + 1);
		glView->getModel()->updateKpsPos(listKps->item(currentIndex)->text().toStdString(), currentIndex);
	}
}
void MainWindow::on_lineNewKps_editingFinished()
{
	// Do nothing...
}

// - Update of current part coordinate
void MainWindow::on_lineKpsX_editingFinished()
{
	int currentIndex = listKps->currentRow();
	if (currentIndex > -1)
	{
		QListWidgetItem* item = listKps->item(currentIndex);
		float X = lineKpsX->text().toFloat();
		glView->updateKpsX(item->text().toStdString(), X);
		glView->getModel()->updateKpsX(item->text().toStdString(), X);
	}
}
void MainWindow::on_lineKpsY_editingFinished()
{
	int currentIndex = listKps->currentRow();
	if (currentIndex > -1)
	{
		QListWidgetItem* item = listKps->item(currentIndex);
		float Y = lineKpsY->text().toFloat();
		glView->updateKpsY(item->text().toStdString(), Y);
		glView->getModel()->updateKpsY(item->text().toStdString(), Y);
	}
}
void MainWindow::on_lineKpsZ_editingFinished()
{
	int currentIndex = listKps->currentRow();
	if (currentIndex > -1)
	{
		QListWidgetItem* item = listKps->item(currentIndex);
		float Z = lineKpsZ->text().toFloat();
		glView->updateKpsZ(item->text().toStdString(), Z);
		glView->getModel()->updateKpsZ(item->text().toStdString(), Z);
	}
}

void MainWindow::on_buttonUpKpsX_clicked()
{
	lineKpsX->setText(QString::number(lineKpsX->text().toFloat() + 0.001));
	emit lineKpsX->editingFinished();
}
void MainWindow::on_buttonDownKpsX_clicked()
{
	lineKpsX->setText(QString::number(lineKpsX->text().toFloat() - 0.001));
	emit lineKpsX->editingFinished();
}
void MainWindow::on_buttonUpKpsY_clicked()
{
	lineKpsY->setText(QString::number(lineKpsY->text().toFloat() + 0.001));
	emit lineKpsY->editingFinished();
}
void MainWindow::on_buttonDownKpsY_clicked()
{
	lineKpsY->setText(QString::number(lineKpsY->text().toFloat() - 0.001));
	emit lineKpsY->editingFinished();
}
void MainWindow::on_buttonUpKpsZ_clicked()
{
	lineKpsZ->setText(QString::number(lineKpsZ->text().toFloat() + 0.001));
	emit lineKpsZ->editingFinished();
}
void MainWindow::on_buttonDownKpsZ_clicked()
{
	lineKpsZ->setText(QString::number(lineKpsZ->text().toFloat() - 0.001));
	emit lineKpsZ->editingFinished();
}

// - Update of current part scaling (Sx,Sy,Sz)
void MainWindow::on_lineKpsSx_editingFinished()
{
	int currentIndex = listKps->currentRow();
	if (currentIndex > -1)
	{
		QListWidgetItem* item = listKps->item(currentIndex);
		float Sx = lineKpsSx->text().toFloat();
		glView->updateKpsSx(item->text().toStdString(), Sx);
		glView->getModel()->updateKpsSx(item->text().toStdString(), Sx);
	}
}
void MainWindow::on_lineKpsSy_editingFinished()
{
	int currentIndex = listKps->currentRow();
	if (currentIndex > -1)
	{
		QListWidgetItem* item = listKps->item(currentIndex);
		float Sy = lineKpsSy->text().toFloat();
		glView->updateKpsSy(item->text().toStdString(), Sy);
		glView->getModel()->updateKpsSy(item->text().toStdString(), Sy);
	}
}
void MainWindow::on_lineKpsSz_editingFinished()
{
	int currentIndex = listKps->currentRow();
	if (currentIndex > -1)
	{
		QListWidgetItem* item = listKps->item(currentIndex);
		float Sz = lineKpsSz->text().toFloat();
		glView->updateKpsSz(item->text().toStdString(), Sz);
		glView->getModel()->updateKpsSz(item->text().toStdString(), Sz);
	}
}
void MainWindow::on_buttonUpKpsSx_clicked()
{
	lineKpsSx->setText(QString::number(lineKpsSx->text().toFloat() + 0.001));
	emit lineKpsSx->editingFinished();
}
void MainWindow::on_buttonDownKpsSx_clicked()
{
	lineKpsSx->setText(QString::number(lineKpsSx->text().toFloat() - 0.001));
	emit lineKpsSx->editingFinished();
}
void MainWindow::on_buttonUpKpsSy_clicked()
{
	lineKpsSy->setText(QString::number(lineKpsSy->text().toFloat() + 0.001));
	emit lineKpsSy->editingFinished();
}
void MainWindow::on_buttonDownKpsSy_clicked()
{
	lineKpsSy->setText(QString::number(lineKpsSy->text().toFloat() - 0.001));
	emit lineKpsSy->editingFinished();
}
void MainWindow::on_buttonUpKpsSz_clicked()
{
	lineKpsSz->setText(QString::number(lineKpsSz->text().toFloat() + 0.001));
	emit lineKpsSz->editingFinished();
}
void MainWindow::on_buttonDownKpsSz_clicked()
{
	lineKpsSz->setText(QString::number(lineKpsSz->text().toFloat() - 0.001));
	emit lineKpsSz->editingFinished();
}

// - Activate special cases of objects
void MainWindow::on_checkKpsNoAz_clicked()
{
	glView->setIsKpsAz(!checkKpsNoAz->isChecked());
}
void MainWindow::on_checkKpsNoSelfOcc_clicked()
{
	glView->setIsKpsSelfOcc(!checkKpsNoSelfOcc->isChecked());
}

// - File save with all parts' coordinates
void MainWindow::on_buttonSaveKps_clicked()
{
	// File info
	size_t found = modelFileName.find_last_of(".");
	string fileNameKps = modelFileName.substr(0, found) + ".kps";

	ofstream out(fileNameKps);
	for (unsigned int i = 0; i < listKps->count(); ++i)
	{
		QListWidgetItem* item = listKps->item(i);
		string id = item->text().toStdString();
		Kp kp = glView->getModel()->getKpsId(id);
		out << id << " " 
			<< QString::number(kp.X).toStdString() << " " << QString::number(kp.Y).toStdString() << " " << QString::number(kp.Z).toStdString() << " "
			<< QString::number(kp.Sx).toStdString() << " " << QString::number(kp.Sy).toStdString() << " " << QString::number(kp.Sz).toStdString()  << endl;
	}
	out.close();
}

void MainWindow::updateKps(map<string,Kp> list_kps)
{
	listKps->clear();
	Kp kp;
	for (unsigned int i = 0; i < list_kps.size(); ++i)
	{
		for (map<string, Kp>::iterator it = list_kps.begin(); it != list_kps.end(); ++it)
		{
			kp = it->second;
			if (kp.pos == i)
				break;
		}
		listKps->insertItem(kp.pos, QString(kp.name.c_str()));
		glView->createKp(kp);
	}
}
