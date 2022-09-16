import contextlib
import functools
import json
import os
import re
import selectors
import signal
import socket
import subprocess
import time
import tempfile


class CMakeServer:

    START_MAGIC_STRING = '[== "CMake Server" ==['
    END_MAGIC_STRING = ']== "CMake Server" ==]'

    def __init__(self, address=None):
        if not address:
            address = tempfile.mktemp()
        self._address = address
        cmd = ['cmake', '-E', 'server', '--experimental']
        cmd.append('--pipe=' + self._address)
        self._process = subprocess.Popen(cmd)
        while not os.path.exists(self._address):
            if self._process.poll() is not None:
                break
            time.sleep(1)

    @property
    def address(self):
        return self._address

    def shutdown(self, timeout=None):
        self._process.send_signal(signal.SIGINT)
        try:
            self._process.wait(timeout)
        except subprocess.TimeoutExpired:
            self._process.kill()


class CMakeClient:

    def __init__(self, address):
        self._socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            self._socket.connect(address)
            self._selector = selectors.DefaultSelector()
            self._selector.register(self._socket, selectors.EVENT_READ)
            self._unpack_buffer = ''

            message = next(self._receive(timeout=5), None)
            assert message
            assert message['type'] == 'hello'
            self._supported_protocol_versions = \
                message['supportedProtocolVersions']
        except Exception:
            self._socket.close()
            raise

    @property
    def supported_protocol_versions(self):
        return self._supported_protocol_versions

    def shutdown(self):
        self._socket.close()

    def _pack(self, payload):
        return '\n'.join([
            CMakeServer.START_MAGIC_STRING,
            json.dumps(payload),
            CMakeServer.END_MAGIC_STRING
        ]) + '\n'

    def _unpack(self, partial):
        self._unpack_buffer += partial
        while self._unpack_buffer:
            packet_start_index = \
                self._unpack_buffer.find(CMakeServer.START_MAGIC_STRING)
            if packet_start_index < 0:
                break
            payload_start_index = \
                packet_start_index + len(CMakeServer.START_MAGIC_STRING)
            payload_end_index = self._unpack_buffer.find(
                CMakeServer.END_MAGIC_STRING, payload_start_index
            )
            if payload_end_index < 0:
                break
            payload = json.loads(
                self._unpack_buffer[payload_start_index:payload_end_index]
            )
            packet_end_index = \
                payload_end_index + len(CMakeServer.END_MAGIC_STRING)
            self._unpack_buffer = self._unpack_buffer[packet_end_index:]

            assert type(payload) is dict
            assert 'type' in payload
            yield payload

    def _send(self, payload):
        packet = self._pack(payload).encode('utf-8')
        written = self._socket.send(packet)
        return len(packet) == written

    def _receive(self, timeout=None):
        if timeout is not None:
            deadline = time.time() + timeout
        while True:
            events = self._selector.select(timeout)
            for key, mask in events:
                assert key.fileobj is self._socket
                assert mask & selectors.EVENT_READ
                partial = self._socket.recv(4096)
                yield from self._unpack(partial.decode('utf-8'))
            if timeout is not None:
                current_time = time.time()
                if current_time >= deadline:
                    break
                timeout = deadline - current_time

    def exchange(
        self,
        request,
        message_callback=None,
        progress_callback=None,
        **kwargs
    ):
        assert self._send(request)
        for response in self._receive(**kwargs):
            if response['type'] == 'signal':
                # Ignore signal responses
                continue
            if response['inReplyTo'] != request['type']:
                continue
            if response['type'] == 'reply':
                return response
            if response['type'] == 'error':
                raise RuntimeError(response['errorMessage'])
            if response['type'] == 'message':
                if message_callback:
                    message_callback(response['message'])
                continue
            if response['type'] == 'progress':
                if progress_callback:
                    message = response['progressMessage']
                    progress = (
                        response['progressMinimum'],
                        response['progressMaximum'],
                        response['progressCurrent']
                    )
                    progress_callback(message, progress)
        raise TimeoutError(
            'Timeout waiting for reply to {!r} request'.format(request)
        )

    def _request_method(self, type_, attributes=None, **kwargs):
        request = {'type': type_}
        if attributes:
            request.update(attributes)
        reply = self.exchange(request, **kwargs)
        return {
            k: v for k, v in reply.items()
            # Drop transport specific attributes
            if k not in ('inReplyTo', 'type', 'cookie')
        }

    def __getattr__(self, name):
        return functools.partial(self._request_method, name)


@contextlib.contextmanager
def server_mode(project_path):
    with tempfile.TemporaryDirectory() as tmpdir:
        cmake_server = CMakeServer(address=os.path.join(tmpdir, 'pipe'))
        try:
            cmake = CMakeClient(cmake_server.address)
            try:
                supported_versions = cmake.supported_protocol_versions
                assert any(v['major'] == 1 for v in supported_versions)
                project_build_path = os.path.join(project_path, 'build')
                cmake.handshake(
                    attributes={
                        'sourceDirectory': os.path.abspath(project_path),
                        'buildDirectory': os.path.abspath(project_build_path),
                        'generator': 'Unix Makefiles',
                        'protocolVersion': {'major': 1}
                    },
                    timeout=5
                )
                yield cmake
            finally:
                cmake.shutdown()
        finally:
            cmake_server.shutdown()
