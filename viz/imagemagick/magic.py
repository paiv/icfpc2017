import os
import subprocess
import tempfile

from glob import glob


class ProcRunner:
    def __init__(self, arg_prefix='--'):
        self.arg_prefix = arg_prefix

    def run(self, *args, input=None, stdout=None, **kwargs):
        args = list(filter(None, args))
        cmd, *args = args
        opts = list(x for k,v in kwargs.items() if v is not None
            for x in ('{}{}'.format(self.arg_prefix, k), str(v)))
        cmd = [cmd] + opts + args

        stdout = stdout or subprocess.STDOUT

        subprocess.run(cmd, input=input, stdout=stdout)


class ImageMagick:
    def __init__(self, engine='convert', **kwargs):
        self.engine = engine
        self.kwargs = kwargs

    def animate(self, frames_dir, target, **kwargs):
        if hasattr(target, 'write'):
            self._process_frames(frames_dir, target, **kwargs)
        else:
            dirs = os.path.dirname(target)
            os.makedirs(dirs, exist_ok=True)

            with open(target, 'w') as fd:
                self._process_frames(frames_dir, fd, **kwargs)

    def _process_frames(self, frames_dir, fd, **kwargs):
        kwargs.update(self.kwargs)

        files = glob(os.path.join(frames_dir, '*'))
        files = '\n'.join('"%s"' % s for s in files)
        files = bytes(files, 'utf-8')

        with tempfile.NamedTemporaryFile() as temp:
            temp.write(files)
            temp.flush()

            proc = ProcRunner(arg_prefix='-')

            proc.run('convert',
                '@{}'.format(temp.name),
                'gif:-',    # produce output to stdout
                input=files,
                stdout=fd,
                **kwargs)
