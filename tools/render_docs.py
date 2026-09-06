"""Reproduce README renders. Optional documentation tool; requires Pillow."""
import argparse
from pathlib import Path
import subprocess
from PIL import Image

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('renderer', type=Path, help='path to the built renderer executable')
args = parser.parse_args()
executable = args.renderer.resolve()
work = root/'build'/'docs'
work.mkdir(parents=True, exist_ok=True)
(root/'docs'/'images').mkdir(parents=True, exist_ok=True)
for name, extra, width, height in [
    ('orbital', [], 1400, 1000),
    ('solid', ['--solid'], 700, 600),
    ('alternate-light', ['--light', '1', '-0.2', '0.5'], 700, 600),
]:
    output = work/(name+'.tga')
    subprocess.run([str(executable), str(root/'assets'/'orbital.obj'),
                    '--output', str(output), '--width', str(width),
                    '--height', str(height), *extra], check=True, cwd=root)
    with Image.open(output) as image:
        image.save(root/'docs'/'images'/(name+'.png'))
    print('Wrote', name+'.png')
