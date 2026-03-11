from training_image_generator import ImageGenerator
import os

if __name__ == "__main__":
    current_dir = os.getcwd()
    generator = ImageGenerator(agent_icon_path=current_dir+"/agent_icons", 
                            minimap_path=current_dir+"/map_layouts",
                            misc_icon_path=current_dir+"/misc_icons",
                            agent_resize_factor=(64, 64))

    for i in range(4):
        data_name = f"{i:04d}"
        generator.generate_data(data_name, is_train=True)
        generator.generate_data(data_name, is_train=False)


    generator.display_class_names()    