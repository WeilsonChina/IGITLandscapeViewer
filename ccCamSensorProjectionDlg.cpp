//##########################################################################
//#                                                                        #
//#                            CLOUDCOMPARE                                #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#          COPYRIGHT: EDF R&D / TELECOM ParisTech (ENST-TSI)             #
//#                                                                        #
//##########################################################################

#include "ccCamSensorProjectionDlg.h"

//local
#include "ccCustomDoubleValidator.h"

//qCC_db
#include <ccCameraSensor.h>

ccCamSensorProjectionDlg::ccCamSensorProjectionDlg(QWidget* parent)
	: QDialog(parent)
	, Ui::CamSensorProjectDialog()
{
	setupUi(this);

	posXEdit->setValidator(new ccCustomDoubleValidator(this));
	posYEdit->setValidator(new ccCustomDoubleValidator(this));
	posZEdit->setValidator(new ccCustomDoubleValidator(this));

	x1rot->setValidator(new ccCustomDoubleValidator(this));
	x2rot->setValidator(new ccCustomDoubleValidator(this));
	x3rot->setValidator(new ccCustomDoubleValidator(this));
	y1rot->setValidator(new ccCustomDoubleValidator(this));
	y2rot->setValidator(new ccCustomDoubleValidator(this));
	y3rot->setValidator(new ccCustomDoubleValidator(this));
	z1rot->setValidator(new ccCustomDoubleValidator(this));
	z2rot->setValidator(new ccCustomDoubleValidator(this));
	z3rot->setValidator(new ccCustomDoubleValidator(this));
}
//=====================================initWithCamSensor==========================================//
//已知相机，对对话框进行初始化
void ccCamSensorProjectionDlg::initWithCamSensor(const ccCameraSensor* sensor)
{
	if( !sensor) return;

	const int precision = sizeof(PointCoordinateType) == 8 ? 12 : 8;

	/*** Position + Orientation ***/
	{
		//center
		const float* C = sensor->getRigidTransformation().getTranslation();
		posXEdit->setText(QString::number(C[0],'f',precision));
		posYEdit->setText(QString::number(C[1],'f',precision));
		posZEdit->setText(QString::number(C[2],'f',precision));

		//rotation matrix
		const ccGLMatrix& rot = sensor->getRigidTransformation();
		{
			const float* mat = rot.data();
			x1rot->setText(QString::number(mat[0] ,'f',precision));
			y1rot->setText(QString::number(mat[1] ,'f',precision));
			z1rot->setText(QString::number(mat[2] ,'f',precision));

			x2rot->setText(QString::number(mat[4] ,'f',precision));
			y2rot->setText(QString::number(mat[5] ,'f',precision));
			z2rot->setText(QString::number(mat[6] ,'f',precision));

			x3rot->setText(QString::number(mat[8] ,'f',precision));
			y3rot->setText(QString::number(mat[9] ,'f',precision));
			z3rot->setText(QString::number(mat[10],'f',precision));
		}
	}

	/*** Intrinsic parameters ***/
	{
		const ccCameraSensor::IntrinsicParameters& iParams = sensor->getIntrinsicParameters();

		focalDoubleSpinBox->setValue(iParams.focal_pix);
		fovDoubleSpinBox->setValue(iParams.vFOV_rad * CC_RAD_TO_DEG);
		arrayWSpinBox->setValue(iParams.arrayWidth);
		arrayHSpinBox->setValue(iParams.arrayHeight);
		pixWDoubleSpinBox->setValue(iParams.pixelSize_mm[0]);
		pixHDoubleSpinBox->setValue(iParams.pixelSize_mm[1]);
		zNearDoubleSpinBox->setValue(iParams.zNear_mm);
		zFarDoubleSpinBox->setValue(iParams.zFar_mm);
		skewDoubleSpinBox->setValue(iParams.skew);
	}

	/*** Distortion / uncertainty ***/
	{
		QString distInfo;
		const ccCameraSensor::LensDistortionParameters::Shared& distParams = sensor->getDistortionParameters();

		if (!distParams){
			distInfo = "No associated distortion /uncertainty model."; //没有相关的畸变/不确定 模型
		}
		else if (distParams->getModel() == ccCameraSensor::SIMPLE_RADIAL_DISTORTION){  //简单径向畸变系数
			const ccCameraSensor::RadialDistortionParameters* rdParams = static_cast<ccCameraSensor::RadialDistortionParameters*>(distParams.data());
			distInfo = "Radial distortion model:\n";
			distInfo += QString("k1 = %1\n").arg(rdParams->k1);
			distInfo += QString("k2 = %1\n").arg(rdParams->k2);
		}
		else if (distParams->getModel() == ccCameraSensor::BROWN_DISTORTION){  //BROWN 径向畸变系数
			const ccCameraSensor::BrownDistortionParameters* bParams = static_cast<ccCameraSensor::BrownDistortionParameters*>(distParams.data());
			distInfo = "Brown distortion / uncertainty model:\n";
			distInfo += "* Radial distortion:\n";
			distInfo += QString("\tK1 = %1\n").arg(bParams->K_BrownParams[0]);
			distInfo += QString("\tK2 = %1\n").arg(bParams->K_BrownParams[1]);
			distInfo += QString("\tK3 = %1\n").arg(bParams->K_BrownParams[2]);
			distInfo += "* Tangential distortion:\n";
			distInfo += QString("\tP1 = %1\n").arg(bParams->P_BrownParams[0]);
			distInfo += QString("\tP2 = %1\n").arg(bParams->P_BrownParams[1]);
			distInfo += "* Linear disparity:\n";
			distInfo += QString("\tA = %1\n").arg(bParams->linearDisparityParams[0]);
			distInfo += QString("\tB = %1\n").arg(bParams->linearDisparityParams[1]);
			distInfo += "* Principal point offset:\n";
			distInfo += QString("\tX = %1\n").arg(bParams->principalPointOffset[0]);
			distInfo += QString("\tY = %1\n").arg(bParams->principalPointOffset[1]);
		}
		else {
			assert(false);
			distInfo = "Unhandled distortion /uncertainty model!";
		}

		distInfoTextEdit->setText(distInfo);
	}
}

//=====================================updateCamSensor===========================================//
void ccCamSensorProjectionDlg::updateCamSensor(ccCameraSensor* sensor)
{
	if (!sensor) return;

	/*** Position + Orientation ***/
	{
		//orientation matrix  // 方向矩阵，相机坐标系的坐标
		ccGLMatrix rot;
		{
			float* mat = rot.data();
			mat[0]  = x1rot->text().toFloat();
			mat[1]  = y1rot->text().toFloat();
			mat[2]  = z1rot->text().toFloat();

			mat[4]  = x2rot->text().toFloat();
			mat[5]  = y2rot->text().toFloat();
			mat[6]  = z2rot->text().toFloat();

			mat[8]  = x3rot->text().toFloat();
			mat[9]  = y3rot->text().toFloat();
			mat[10] = z3rot->text().toFloat();
		}

		//center  //相机坐标系中心
		CCVector3 C(static_cast<PointCoordinateType>(posXEdit->text().toDouble()),
					static_cast<PointCoordinateType>(posYEdit->text().toDouble()),
					static_cast<PointCoordinateType>(posZEdit->text().toDouble()));

		//设置平移向量
		rot.setTranslation(C);
		sensor->setRigidTransformation(rot);
	}

	/*** Intrinsic parameters ***/  //设置内参数
	{
		ccCameraSensor::IntrinsicParameters iParams;

		iParams.focal_pix = static_cast<float>(focalDoubleSpinBox->value()); //焦距
		iParams.vFOV_rad = static_cast<float>(fovDoubleSpinBox->value() * CC_DEG_TO_RAD); //视场角
		iParams.arrayWidth = arrayWSpinBox->value(); //图像宽度
		iParams.arrayHeight = arrayHSpinBox->value();//图像高度
		iParams.pixelSize_mm[0] = static_cast<float>(pixWDoubleSpinBox->value()); //像素大小
		iParams.pixelSize_mm[1] = static_cast<float>(pixHDoubleSpinBox->value());//像素大小
		iParams.zNear_mm = static_cast<float>(zNearDoubleSpinBox->value()); //近侧裁剪面
		iParams.zFar_mm = static_cast<float>(zFarDoubleSpinBox->value()); //远侧裁剪面
		iParams.skew = static_cast<float>(skewDoubleSpinBox->value());  //错切稀疏

		sensor->setIntrinsicParameters(iParams);  //设置内参数
	}


	/*** Distortion / uncertainty ***/

	//read only for now

}
