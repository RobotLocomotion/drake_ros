import glob
import os


def get_packages_with_prefixes(prefixes=None):
    if prefixes is None:
        prefixes = os.environ['CMAKE_PREFIX_PATH'].split(os.pathsep)
        prefixes = [prefix for prefix in prefixes if prefix]
    suffixes = 'Config.cmake', '-config.cmake'
    return {
        os.path.basename(path)[:-len(suffix)]: prefix
        for prefix in prefixes for suffix in suffixes
        for path in glob.glob(
            '{}/**/*{}'.format(
                prefix, suffix),
            recursive=True
        )
    }
