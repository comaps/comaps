import sys
import os
import shutil
import zipfile

def main():
  if len(sys.argv) != 4:
    print("Usage: tif2hgt.py <src aster path> <tmp path> <dest srtm path>")
    return

  aster_path = str(sys.argv[1])
  tmp_path = str(sys.argv[2])
  srtm_path = str(sys.argv[3])

  for file in os.listdir(aster_path):
    if file.endswith(".zip"):
      dest_dir = tmp_path + '/' + file
      with zipfile.ZipFile(aster_path + '/' + file, 'r') as zip_ref:
        os.mkdir(dest_dir)
        zip_ref.extractall(dest_dir)

        for tif_file in os.listdir(dest_dir):
          # Sample: ASTGTMV003_N61E010_dem.tif
          if tif_file.endswith("dem.tif"):
            print("Process: " + tif_file[11:18])

            arch_name = tif_file[11:18] + '.SRTMGL1.hgt'
            out_file = srtm_path + '/' + arch_name
            os.system('gdal_translate -of SRTMHGT ' + dest_dir + '/' + tif_file + ' ' + out_file)

            zipfile.ZipFile(out_file + '.zip', mode='w', compression=zipfile.ZIP_DEFLATED).write(out_file, arch_name)

            os.remove(out_file)
            shutil.rmtree(dest_dir)
            break

main()
