# Render
Automatic generation of synthetic data from rigid 3D graphics models:
- Viewpoints (azimuth, elevation and in-plane rotation
- Keypoints if annotated (given models already contain keypoints (*.kps files)

Requirements:
- Visual Studio 2013
- VS Qt Addin 1.2.5
- Qt 5.3.1/5.3.2 (any version >= 5.1 up to 5.3 should be also fine)
- Update data\config.txt file with the desired paths

In the app:
- View tab:
  - Load Model: Loads any desired 3D models
- 3D Label tab (Prototype not working)
- Sampling: 
  - Camera Range: (not implemented)
  - View size step: important data for synthetic data generation
  - Interval: Generates data with only discretised value (e.g. 1,4,5)
  - Range: Specificy 2 values and random values between the range will be generated (e.g. 0,359)  Note: Azimuth always 360º in range and defined the granularity (e.g every 1º)
  - Update Model (class folder that contains subfolders of 3D models), Output and Background folders
  - Preview: just for visualisation > Save View: one single test image
  - Run Script: generates synthetic data based on all parameters
- Keypoint: 
  - Save Keypoints: saves current keypoints with model's name + hardcoded extension (update all places with ".kps" with the desired extension)
  
Hardcoded automatic generations for as many classes as desired can be added in the following functions, since manual rendering only works for 1 single class (its subfolders):
- MainWindow::on_buttonScript_clicked()

Note: .obj classes are preferred to avoid unexpected visualisations
