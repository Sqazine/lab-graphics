# https://mp.weixin.qq.com/s?__biz=MzU0NjgzMDIxMQ==&mid=2247490152&idx=2&sn=852db92d636b04481b3680d6750d5ce3&chksm=fb56f884cc2171928498ec6a3aededbc83e6157dab8dc0f20b494824d4daf8479e9d9907d102&token=128401448&lang=zh_CN#rd
# https://github.com/fengzhenHIT/OpenCV_Projects

# how to run: 
# streamlit run .\cv_image_toon.py

import cv2
import streamlit as st
import numpy as np
from PIL import Image


def cartoonization(img, cartoon):
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    if cartoon == "铅笔素描":
        value = st.sidebar.slider(
            "Tune the brightness of your sketch (the higher the value, the brighter your sketch)", 0.0, 300.0, 250.0)
        kernel = st.sidebar.slider(
            'Tune the boldness of the edges of your sketch (the higher the value, the bolder the edges)', 1, 99, 25, step=2)
        gray_blur = cv2.GaussianBlur(gray, (kernel, kernel), 0)
        cartoon = cv2.divide(gray, gray_blur, scale=value)

    elif cartoon=="细节强化":

        smooth=st.sidebar.slider('Tune the smoothness level of the image (the higher the value, the smoother the image)', 3, 99, 5, step=2)
        kernel=st.sidebar.slider('Tune the sharpness of the image (the lower the value, the sharper it is)', 1, 21, 3, step =2)
        edge_preserve=st.sidebar.slider('Tune the color averaging effects (low: only similar colors will be smoothed, high: dissimilar color will be smoothed)', 0.0, 1.0, 0.5)

        gray_blur = cv2.medianBlur(gray, kernel)
        edges=cv2.adaptiveThreshold(gray_blur,255,cv2.ADAPTIVE_THRESH_MEAN_C,cv2.THRESH_BINARY,9,9)
        color=cv2.detailEnhance(img,sigma_s=smooth,sigma_r=edge_preserve)
        cartoon=cv2.bitwise_and(color,color,mask=edges)

    elif cartoon=="双边过滤器":

        smooth = st.sidebar.slider('Tune the smoothness level of the image (the higher the value, the smoother the image)', 3, 99, 5, step=2)
        kernel = st.sidebar.slider('Tune the sharpness of the image (the lower the value, the sharper it is)', 1, 21, 3, step =2)
        edge_preserve = st.sidebar.slider('Tune the color averaging effects (low: only similar colors will be smoothed, high: dissimilar color will be smoothed)', 1, 100, 50)

        gray_blur=cv2.medianBlur(gray,kernel)
        edges=cv2.adaptiveThreshold(gray_blur,255,cv2.ADAPTIVE_THRESH_MEAN_C,cv2.THRESH_BINARY,9,9)
        color=cv2.bilateralFilter(img,smooth,edge_preserve,smooth)
        cartoon=cv2.bitwise_and(color,color,mask=edges)


    elif cartoon=="铅笔边缘":
        kernel = st.sidebar.slider('Tune the sharpness of the sketch (the lower the value, the sharper it is)', 1, 99, 25, step=2)
        laplacian_filter = st.sidebar.slider('Tune the edge detection power (the higher the value, the more powerful it is)', 3, 9, 3, step =2)
        noise_reduction = st.sidebar.slider('Tune the noise effects of your sketch (the higher the value, the noisier it is)', 10, 255, 150)
        
        gray=cv2.medianBlur(gray,kernel)
        edges=cv2.Laplacian(gray,-1,ksize=laplacian_filter)
        edges_inv=255-edges

        dummy,cartoon=cv2.threshold(edges_inv,noise_reduction,255,cv2.THRESH_BINARY)

    return cartoon

file = st.sidebar.file_uploader(
    "请上传图片", type=["jpg", "png"])

if file is None:
    st.text("还未上传图片")
else:
    image = Image.open(file)
    img = np.array(image)

    option = st.sidebar.selectbox(
        '选择卡通滤波器',
        ('铅笔素描', '细节强化', '铅笔边缘', '双边过滤器'))

    st.text("原始图像")
    st.image(image, use_column_width=True)

    st.text("卡通化图像")
    cartoon = cartoonization(img, option)

    st.image(cartoon, use_column_width=True)
