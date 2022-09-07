# tinyCamML

This is a machine learning pipeline that goes from training a model, quantization, to deployment on an MCU for in-the-field use. 

## Model Creation

### Binary Classification

Included are notebooks for training, testing, and quantizing MobileNetV2 based models for binary classification of flooded/not_flooded.

#### mobilenetv2_training.ipynb
A training notebook to create a binary classification model based on mobilenetv2, to be used with input images of 224x224. 

#### ModelTester.ipynb
A testing notebook to test any models you have trained. Allows you to load a model, use it to generate predictions on images you choose, generate f1 scores, generate confusion-matrices, and generate GradCAM overlays of predictions from the model.

#### model_converter.ipynb
This notebook lets you load a model created using the training notebook and convert it to the .tflite format. It also allows you to load a .tflite model and make predictions with it. This could be used with a segmentation model but has not been tested yet.

### Segmentation

#### Unet_model_trainer.ipynb
A training notebook to be used with images that have been labeled with [Doodler](https://github.com/Doodleverse/dash_doodler).

Use Doodler to "doodle" images with their classes. For this project the following classes were used: water, road, building, sidewalk, people, other. Use class_remapper.ipynb to consolodate classes to: water, road, neither. 

Use the "gen_images_and_labels.py" utility from Doodler to create two folders containing the images and labels from your doodled images. Move these folders to data->segmentation as shown in the directory structure below.

Directory structure for training:
```
──data─┬─classification─┬──flooded
       │                └──not_flooded
       │
       └─segmentation───┬──images──images
                        └──labels──labels
```

## Model Deployment

This portion of the project is not yet complete. Further developments will be pushed to the new repository [here](https://github.com/JoeWTG/SunnyD-SD-2022). 

Implementation currently uses [EloquentTinyML](https://github.com/eloquentarduino/EloquentTinyML) to run model inference on the ESP32-CAM.

Use the unix script found in model_converter.ipynb to convert the .tflite model into a C array.

Rename the converted model to "modeldata.h" and place in the same directory as the ESP32 Inference code. 

## Utilities

#### filename_scraper.ipynb
This notebook will create a CSV of all files located in a directory. Useful for creating a csv of the filenames of your labeled images for tracking.

#### class_remapper.ipynb
This notebook can be used to remap classes from labeled doodles. Open the notebook, choose an image, configure the remapper, and remap.
Currently it is a manual process. Future version of this util will allow a user to select a directory of images to remap.

## Camera Module

Included in this repository is code for the ESP32-CAM that will capture and save images to a microSD based on a user-defined interval.

To use, upload the code to the ESP32-CAM and connect using a Bluetooth Serial app on a smart phone. From here you can configure the date&time, capture interval, and image index. 

### [Presentation](https://docs.google.com/presentation/d/1H9vci88QYEPyQeXLo3Sxy1dpsh4Yb4WI/)

[Continued work by the NCSU ECE Senior Design team will be hosted here.](https://github.com/JoeWTG/SunnyD-SD-2022)
