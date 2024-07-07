import pygame
import time

# Initialize Pygame
pygame.init()
screen = pygame.display.set_mode((800, 480))
pygame.display.set_caption("Speedometer Simulator")
font = pygame.font.Font(None, 74)
clock = pygame.time.Clock()

# File to read simulated speeds from
file_path = 'speed_output.txt'

def read_speed_from_file():
    gps_speed = 0.0
    hall_speed = 0.0
    try:
        with open(file_path, 'r') as f:
            lines = f.readlines()
            for line in lines:
                parts = line.split()
                if parts[0] == 'GPS':
                    gps_speed = float(parts[1])
                elif parts[0] == 'HALL':
                    hall_speed = float(parts[1])
    except FileNotFoundError:
        print(f"File {file_path} not found.")
    except Exception as e:
        print(f"Error reading from file: {e}")
    return gps_speed, hall_speed

try:
    while True:
        # Read simulated speeds
        gps_speed, hall_speed = read_speed_from_file()

        # Display speeds
        screen.fill((0, 0, 0))
        gps_text = font.render(f"GPS Speed: {gps_speed:.2f} km/h", True, (255, 255, 255))
        hall_text = font.render(f"Hall Speed: {hall_speed:.2f} km/h", True, (255, 255, 255))
        screen.blit(gps_text, (100, 150))
        screen.blit(hall_text, (100, 250))
        pygame.display.flip()

        # Sleep for a while
        time.sleep(1)
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                raise KeyboardInterrupt

except KeyboardInterrupt:
    pygame.quit()
