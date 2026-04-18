'''
Simple script for taking in transparent agent pngs and splitting into enemy & friendly and adding the appropriate color circular background
It takes as a command line argument the relative path for the transparent icons, and outputs to agent_icons
'''

import cv2
import numpy as np
import os
import sys
from PIL import Image
import torchvision
import random

if (len(sys.argv)) != 2:
    print("Proper usage: python agentpreparation.py <path to transparent agent images>")
    sys.exit()

def load_image(image_path):

            if os.path.isfile(image_path):

                full_image_path = os.path.basename(image_path)
                split = os.path.splitext(full_image_path)
                basefile = split[0]
                extension = split[1]

                if extension.lower() == ".png":
                    image = Image.open(image_path)
                    image = torchvision.transforms.ToTensor()(image).numpy()
                    return (image, basefile)
                else:
                    raise ValueError(f"Unsupported image format: {extension}")

def load_results(folder_path, array, resize_factor=None):
    for path in os.listdir(folder_path):
        path = os.path.join(folder_path, path)
        result = load_image(path)
        if result:
            img, label = result
            img = img.transpose(1, 2, 0) # Puts everything in HWC format for cv2
            if resize_factor is not None:
                img = cv2.resize(img, resize_factor)
            array.append((img, label))


def draw_background(image, color):

    def create_circular_mask(h, w,  radius_offset=0):
        #offset = int(min(h, w) / 40)
        #center = (int(w/2) + offset, int(h/2) - offset)
        center = (int(w/2), int(h/2))
        
        radius = min(center[0], center[1], w-center[0], h-center[1]) - radius_offset

        Y, X = np.ogrid[:h, :w]
        dist_from_center = np.sqrt((X - center[0])**2 + (Y-center[1])**2)

        mask = dist_from_center <= radius

        return mask, center, radius

    src = image.copy()
    h, w = src.shape[:2]

    transparent_mask = image[:, :, 3] == 0
    src[transparent_mask] = color

    radius_offset = int(min(h, w) / 9)
    mask, center, radius = create_circular_mask(h, w, radius_offset=radius_offset)
    src[~mask] = 0

    thickness = int(min(h, w) / 64)
    src = cv2.circle(src, center, radius, color, thickness)

    return src


unprepared_agent_icons_folder = sys.argv[1]

icons = []
load_results(unprepared_agent_icons_folder, icons)
current_dir = os.getcwd()
write_dir = os.path.join(current_dir, "agent_icons")

prepared_icons = []

# Colors in BGRA format
friendly_color = (213/255, 251/255, 104/255, 1.0)
enemy_color = (34/255, 35/255, 185/255, 1.0)

for icon, label in icons:
    icon = cv2.cvtColor(icon, cv2.COLOR_BGRA2RGBA)

    # Create variation for friendly side
    prepared_icon = draw_background(icon, friendly_color)
    prepared_icons.append((prepared_icon, label + "_friendly"))

    # Create variation for enemy side
    prepared_icon = draw_background(icon, enemy_color)
    prepared_icons.append((prepared_icon, label + "_enemy"))


for icon, label in prepared_icons:

     write_path = os.path.join(write_dir, label + ".png")
     image_uint8 = (np.clip(icon, 0, 1) * 255).astype(np.uint8) # Don't ask me why this part is here
     success = cv2.imwrite(write_path, image_uint8)
     if not success:
         raise IOError(f"Failed to write image to {image_path}")
