import os
from PIL import Image
import torchvision
import cv2
import random
import numpy as np

"""
agent_icon_path: relative path to the folder with agent icons
minimap_path: relative path to the folder with map layouts
agent_resize_factor: the size that the agent icons should be resized to when drawn on the minimap (tuple in form: height, width)

Note: images are kept internally in height, width, channels format for ease of use with cv2
"""

class ImageGenerator:
    
    def __init__(self,
                 agent_icon_path: str,
                 minimap_path: str,
                 agent_resize_factor=(64,64)):
        self.agent_icon_path = agent_icon_path
        self.map_layout_path = minimap_path
        self.agent_resize_factor = agent_resize_factor

        self.agent_icons = []
        self.map_layouts = []

        def load_results(folder_path, array):
            for path in os.listdir(folder_path):
                path = os.path.join(folder_path, path)
                result = self.load_image(path)
                if result:
                    img, label = result
                    img = img.transpose(1, 2, 0) # Puts everything in HWC format for cv2
                    array.append((img, label))

        load_results(self.agent_icon_path, self.agent_icons)
        load_results(self.map_layout_path, self.map_layouts)

    # Takes in an image path and returns the image (as a numpy array) and the filename (without extension)
    def load_image(self, image_path):

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

    # Pick a random minimap and return it as a numpy array
    def generate_map(self):
        minimap, _ = random.choice(self.map_layouts)
        minimap = minimap.copy()
        return minimap

    # Takes in a minimap and a number of agents to draw, and returns the minimap with the agents drawn on it, as well as the positions of the agents
    def draw_agents(self, minimap, num_agents_to_draw):


        position_data = []

        for _ in range(num_agents_to_draw):
            agent_icon, agent_name = random.choice(self.agent_icons)
            agent_icon = self.square_to_circle_with_border(image=agent_icon)

            # Get dimensions of the minimap and agent icon
            minimap_height, minimap_width, _ = minimap.shape
            icon_height, icon_width, _ = agent_icon.shape

            # Check if agent icon fits within minimap (should always be the case)
            if icon_width > minimap_width or icon_height > minimap_height:
                continue

            # Randomly select a position for the agent on the minimap
            x = random.randint(0, minimap_width - icon_width)
            y = random.randint(0, minimap_height - icon_height)

            # Overlay the agent icon onto the minimap at the selected position
            result = self.overlay_transparent(minimap, agent_icon, x, y, self.agent_resize_factor)
            if result is not None:
                minimap = result
                height, width = self.agent_resize_factor
                position_data.append({
                    "name": agent_name,
                    "coordinates": (x, y),
                    "height": height,
                    "width": width
                })

        return minimap, position_data


    # Takes in a numpy array (image) and returns a circular version as a numpy array
    def square_to_circle_with_border(self, image, border_color=None):

        image = image.transpose(2, 1, 0)

        _, height, width = image.shape
        mask = self.create_circular_mask(height, width)

        # Apply mask to alpha channel
        masked_img = image.copy()
        masked_img[3][~mask] = 0

        masked_img = masked_img.transpose(2, 1, 0)

        return masked_img

    def create_circular_mask(self, h, w, center=None, radius=None):

        if center is None: # use the middle of the image
            center = (int(w/2), int(h/2))
        if radius is None: # use the smallest distance between the center and image walls
            radius = min(center[0], center[1], w-center[0], h-center[1])

        Y, X = np.ogrid[:h, :w]
        dist_from_center = np.sqrt((X - center[0])**2 + (Y-center[1])**2)

        mask = dist_from_center <= radius
        return mask

    # Takes in a background and overlays an image on top of it (the overlay must be RGBA). X and Y are the position the overlay should appear on the background
    def overlay_transparent(self, background, overlay, x, y, overlay_size=None):
        """
        @brief      Overlays a transparant PNG onto another image using CV2

        @param      background_img    The background image
        @param      img_to_overlay_t  The transparent image to overlay (has alpha channel)
        @param      x                 x location to place the top-left corner of our overlay
        @param      y                 y location to place the top-left corner of our overlay
        @param      overlay_size      The size to scale our overlay to (tuple), no scaling if None

        @return     Background image with overlay on top
        """

        overlay = cv2.cvtColor(overlay, cv2.COLOR_RGBA2BGRA) # Convert RGBA to BGRA
        overlay = cv2.resize(overlay, overlay_size) if overlay_size is not None else overlay

        background_width = background.shape[1]
        background_height = background.shape[0]

        if x >= background_width or y >= background_height:
            return None

        h, w = overlay.shape[0], overlay.shape[1]
        if h == 0 or w == 0:
            return None

        if x + w > background_width:
            w = background_width - x
            overlay = overlay[:, :w]

        if y + h > background_height:
            h = background_height - y
            overlay = overlay[:h]

        if overlay.shape[2] < 4:
            overlay = np.concatenate(
                [
                    overlay,
                    np.ones((overlay.shape[0], overlay.shape[1], 1), dtype=overlay.dtype)
                ],
                axis=2,
            )
        overlay_image = overlay[..., :4]
        mask = overlay[..., 3:]

        #print(background.shape, overlay.shape, overlay_image.shape, y, y+h, x, x+h)
        x = max(x, 0)
        y = max(y, 0)
        background[y:y+h, x:x+w] = (1.0 - mask) * background[y:y+h, x:x+w] + mask * overlay_image

        return background

    # Takes in a background image (should be a minimap) and draws bounding boxes based on the passed in positions
    def draw_boxes_on_image(self, background, positions):

        for position in positions:
            x, y = position["coordinates"]
            width = position["width"]
            height = position["height"]

            top_left = (x, y)
            bottom_right = (x + width, y + height)

            # This shouldn't be needed, but y'know.
            if top_left[0] < 0 or top_left[1] < 0 or bottom_right[0] > background.shape[0] or bottom_right[1] > background.shape[1]:
                continue

            purple = (128, 0, 128)
            cv2.rectangle(background, top_left, bottom_right, purple, 2)

        return background

    # Returns a dict mapping class names to class ids
    def get_class_ids(self):
        class_ids = {}
        for i, (icon, name) in enumerate(self.agent_icons):
            class_ids[name] = i
        return class_ids

    def output_image_and_label(self, image, positions, file_name, is_train=True):

        class_ids = self.get_class_ids()

        current_dir = os.getcwd()

        image_height = image.shape[0]
        image_width = image.shape[1]

        label_subdir = os.path.join("dataset", "labels", "train" if is_train else "val")
        image_subdir = os.path.join("dataset", "images", "train" if is_train else "val")

        # Make sure directories exist
        os.makedirs(os.path.join(current_dir, label_subdir), exist_ok=True)
        os.makedirs(os.path.join(current_dir, image_subdir), exist_ok=True)

        # Write label data
        label_path = os.path.join(current_dir, label_subdir, file_name + ".txt")
        with open(label_path, "w") as file:
            for position in positions:
                class_id = class_ids[position["name"]]

                box_width = position["width"]
                box_height = position["height"]

                x_center = (position["coordinates"][0] + box_width / 2) / image_width
                y_center = (position["coordinates"][1] + box_height / 2) / image_height

                width = box_width / image_width
                height = box_height / image_height

                # x_center = max(0, min(1, x_center))
                # y_center = max(0, min(1, y_center))
                # width = max(0.01, min(1, width))  # Ensure minimum width to avoid zero-sized boxes
                # height = max(0.01, min(1, height))  # Ensure minimum height to avoid zero-sized boxes

                # Only write valid bounding boxes
                if width > 0 and height > 0 and 0 <= x_center <= 1 and 0 <= y_center <= 1:
                    file.write(f"{class_id} {x_center} {y_center} {width} {height}\n")

        # Save image data 
        image_path = os.path.join(current_dir, image_subdir, file_name + ".png")
        # Ensure image values are uint8 for cv2.imwrite
        image_uint8 = (np.clip(image, 0, 1) * 255).astype(np.uint8) # Don't ask me why this part is here
        success = cv2.imwrite(image_path, image_uint8)
        if not success:
            raise IOError(f"Failed to write image to {image_path}")



    # Takes in the name of the files you want to create and outputs an image and txt file with that name
    def generate_data(self, data_name, is_train=True):
 
        while True:

            minimap = self.generate_map()

            minimap, positions = self.draw_agents(minimap, num_agents_to_draw=10)

            # Retry if no valid positions were generated
            if minimap is None or len(positions) == 0:
                continue

            self.output_image_and_label(minimap, positions, data_name, is_train=is_train)
            break
        # (For later) add abilities

    # Displays the ID and name of each class for putting in the yaml file
    def display_class_names(self):
        class_ids = self.get_class_ids()
        print("Class ID/Name to copy into yaml file:")
        print("names:")
        for name, class_id in class_ids.items():
            print(f"\t{class_id}: {name}")
  

        


        