import pygame
import sys

# Initialize Pygame
pygame.init()
screen = pygame.display.set_mode((800, 480))
pygame.display.set_caption("RPM Simulator")
font = pygame.font.Font(None, 74)
clock = pygame.time.Clock()

def read_rpm_from_input():
    rpm = 0.0
    try:
        input_data = sys.stdin.readline().strip()  # Read one line at a time
        parts = input_data.split(':')
        if parts[0] == 'RPM':
            rpm = float(parts[1])
    except Exception as e:
        print(f"Error reading from input: {e}")
    return rpm

try:
    while True:
        # Read RPM data
        rpm = read_rpm_from_input()

        # Display RPM
        screen.fill((0, 0, 0))
        rpm_text = font.render(f"RPM: {rpm:.2f}", True, (255, 255, 255))
        screen.blit(rpm_text, (100, 200))
        pygame.display.flip()

        # Check for quit events
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                raise KeyboardInterrupt

        clock.tick(60)  # Limit to 60 FPS

except KeyboardInterrupt:
    pygame.quit()
