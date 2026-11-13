import 'package:flutter/material.dart';

import '../../core/models.dart';
import '../../services/discovery.dart';
import '../../state/app_controller.dart';
import '../theme.dart';

class ConnectPage extends StatefulWidget {
  const ConnectPage({super.key, required this.go});
  final void Function(int) go;
  @override
  State<ConnectPage> createState() => _ConnectPageState();
}

class _ConnectPageState extends State<ConnectPage> {
  late final TextEditingController _host;
  late final TextEditingController _port;
  late final TextEditingController _user;
  final _pass = TextEditingController();
  bool _testing = false;
  String? _testResult;
  bool _testOk = false;
  bool _showPass = false;
  bool _scanning = false;
  double _scanProgress = 0;
  List<FoundServer>? _found;

  Future<void> _scan() async {
    final port = int.tryParse(_port.text.trim()) ?? 5000;
    if (port < 1 || port > 65535) {
      showMsg(context, 'Informe uma porta entre 1 e 65535.');
      return;
    }
    setState(() {
      _scanning = true;
      _scanProgress = 0;
      _found = null;
    });
    try {
      final r = await discoverFtpServers(
        port: port,
        onProgress: (v) {
          if (mounted) setState(() => _scanProgress = v);
        },
      );
      if (!mounted) return;
      setState(() => _found = r);
      if (r.length == 1) {
        _host.text = r.first.host;
        _port.text = '${r.first.port}';
      }
      if (r.isEmpty) {
        showMsg(context, 'Nenhum FTP encontrado. Abra o FTPD no Switch e tente novamente.');
      } else {
        showMsg(context, '${r.length} dispositivo(s) encontrado(s). Toque em Conectar.');
      }
    } catch (e) {
      if (mounted) showMsg(context, 'Falha ao procurar na rede: $e');
    } finally {
      if (mounted) setState(() => _scanning = false);
    }
  }

  @override
  void initState() {
    super.initState();
    _host = TextEditingController();
    _port = TextEditingController(text: '5000');
    _user = TextEditingController();
  }

  bool _loaded = false;
  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (_loaded) return;
    _loaded = true;
    final s = AppScope.of(context).settings;
    _host.text = s.host;
    _port.text = '${s.port}';
    _user.text = s.user;
  }

  @override
  void dispose() {
    _host.dispose();
    _port.dispose();
    _user.dispose();
    _pass.dispose();
    super.dispose();
  }

  ConnectionSettings? _read(AppController app) {
    var host = _host.text.trim();
    var portText = _port.text.trim();

    // Aceita também o formato colado 192.168.1.20:5000.
    final endpoint = RegExp(r'^(\d{1,3}(?:\.\d{1,3}){3}):(\d{1,5})$')
        .firstMatch(host);
    if (endpoint != null) {
      host = endpoint.group(1)!;
      portText = endpoint.group(2)!;
      _host.text = host;
      _port.text = portText;
    }

    final octets = host.split('.');
    final validIpv4 = octets.length == 4 &&
        octets.every((part) {
          final value = int.tryParse(part);
          return value != null && value >= 0 && value <= 255;
        });
    final validHostname = RegExp(r'^[A-Za-z0-9.-]+$').hasMatch(host) &&
        RegExp(r'[A-Za-z]').hasMatch(host) &&
        !host.startsWith('.') &&
        !host.endsWith('.') &&
        !host.contains('..');
    final port = int.tryParse(portText);
    if ((!validIpv4 && !validHostname) ||
        port == null ||
        port < 1 ||
        port > 65535) {
      final message = RegExp(r'^\d+$').hasMatch(host)
          ? 'O IP está sem pontos. Exemplo: 192.168.1.25. Deixe a porta 5000 no campo próprio.'
          : 'Informe um IP válido (ex.: 192.168.1.25) e uma porta entre 1 e 65535.';
      showMsg(context, message);
      return null;
    }
    return app.settings.copy()
      ..host = host
      ..port = port
      ..user = _user.text.trim()
      ..password = _pass.text;
  }

  Future<void> _test(AppController app) async {
    final settings = _read(app);
    if (settings == null) return;
    setState(() {
      _testing = true;
      _testResult = null;
    });
    try {
      final result = await app.testConnection(settings);
      if (!mounted) return;
      setState(() {
        _testOk = true;
        _testResult = result;
      });
    } catch (e) {
      if (!mounted) return;
      setState(() {
        _testOk = false;
        _testResult = 'Falha: $e';
      });
    } finally {
      if (mounted) setState(() => _testing = false);
    }
  }

  Future<void> _connect(AppController app) async {
    FocusScope.of(context).unfocus();
    final settings = _read(app);
    if (settings == null) return;

    setState(() {
      _testResult = null;
      _testing = true;
    });
    try {
      await app.connect(settings);
      if (!mounted) return;
      if (app.connected) {
        widget.go(2);
      } else {
        final message = app.status.trim().isEmpty
            ? 'Não foi possível conectar. Confira o IP, a porta 5000 e se o FTPD está aberto no Switch.'
            : app.status;
        setState(() {
          _testOk = false;
          _testResult = message;
        });
        showMsg(context, message);
      }
    } catch (e) {
      if (!mounted) return;
      final message = 'Falha ao conectar: $e';
      setState(() {
        _testOk = false;
        _testResult = message;
      });
      showMsg(context, message);
    } finally {
      if (mounted) setState(() => _testing = false);
    }
  }

  Future<void> _selectServer(FoundServer server, AppController app) async {
    _host.text = server.host;
    _port.text = '${server.port}';
    FocusScope.of(context).unfocus();
    await _connect(app);
  }

  @override
  Widget build(BuildContext context) {
    final app = AppScope.of(context);
    final connected = app.connected;
    final canConnect = !connected && !app.busy && !_testing;
    return Center(
      child: ConstrainedBox(
        constraints: const BoxConstraints(maxWidth: 560),
        child: ListView(
          padding: const EdgeInsets.fromLTRB(18, 16, 18, 24),
          children: [
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Row(
                  children: [
                    Container(
                      width: 46,
                      height: 46,
                      decoration: BoxDecoration(
                        color: (connected ? Colors.greenAccent : kBlue)
                            .withValues(alpha: .15),
                        borderRadius: BorderRadius.circular(14),
                      ),
                      child: Icon(
                        connected ? Icons.wifi : Icons.wifi_find,
                        color: connected ? Colors.greenAccent : kBlue,
                        size: 25,
                      ),
                    ),
                    const SizedBox(width: 12),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            connected ? 'Switch conectado' : 'Conectar ao Switch',
                            style: const TextStyle(
                              fontWeight: FontWeight.w700,
                              fontSize: 16,
                            ),
                          ),
                          const SizedBox(height: 4),
                          Text(
                            app.status,
                            style: const TextStyle(color: Colors.white70),
                            maxLines: 2,
                            overflow: TextOverflow.ellipsis,
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 18),
            TextField(
              controller: _host,
              enabled: !connected && !app.busy,
              keyboardType: TextInputType.url,
              textInputAction: TextInputAction.next,
              decoration: const InputDecoration(
                labelText: 'Endereço IP do Switch',
                hintText: '192.168.1.25',
                helperText: 'Digite o IP com os pontos. A porta fica abaixo.',
                prefixIcon: Icon(Icons.wifi),
              ),
            ),
            const SizedBox(height: 8),
            OutlinedButton.icon(
              onPressed: (connected || app.busy || _scanning) ? null : _scan,
              icon: _scanning
                  ? const SizedBox(
                      width: 18,
                      height: 18,
                      child: CircularProgressIndicator(strokeWidth: 2),
                    )
                  : const Icon(Icons.radar),
              label: Text(_scanning
                  ? 'Procurando na rede… ${(_scanProgress * 100).round()}%'
                  : 'Procurar Switch na rede'),
            ),
            if (_scanning) ...[
              const SizedBox(height: 8),
              ClipRRect(
                borderRadius: BorderRadius.circular(8),
                child: LinearProgressIndicator(value: _scanProgress, minHeight: 5),
              ),
            ],
            if (_found != null) ...[
              const SizedBox(height: 14),
              Text(
                'DISPOSITIVOS ENCONTRADOS',
                style: Theme.of(context).textTheme.labelMedium?.copyWith(
                      color: Colors.white60,
                      letterSpacing: 1.1,
                      fontWeight: FontWeight.w700,
                    ),
              ),
              const SizedBox(height: 6),
              if (_found!.isEmpty)
                const Card(
                  child: Padding(
                    padding: EdgeInsets.all(14),
                    child: Text(
                      'Nenhum FTP encontrado. Confira se o FTPD está aberto no Switch e se os dois aparelhos estão na mesma rede Wi-Fi.',
                    ),
                  ),
                )
              else
                for (final server in _found!)
                  Card(
                    child: ListTile(
                      contentPadding: const EdgeInsets.symmetric(
                        horizontal: 14,
                        vertical: 6,
                      ),
                      leading: const CircleAvatar(
                        backgroundColor: Color(0x223B82F6),
                        child: Icon(Icons.sports_esports, color: kBlue),
                      ),
                      title: Text(
                        '${server.host}:${server.port}',
                        style: const TextStyle(fontWeight: FontWeight.w700),
                      ),
                      subtitle: Text(server.banner.isEmpty
                          ? 'Servidor FTP do Switch'
                          : server.banner),
                      onTap: canConnect
                          ? () => _selectServer(server, app)
                          : null,
                      trailing: FilledButton.tonal(
                        onPressed: canConnect
                            ? () => _selectServer(server, app)
                            : null,
                        child: const Text('Conectar'),
                      ),
                    ),
                  ),
            ],
            const SizedBox(height: 16),
            TextField(
              controller: _port,
              enabled: !connected && !app.busy,
              keyboardType: TextInputType.number,
              decoration: const InputDecoration(
                labelText: 'Porta FTP',
                hintText: '5000',
                prefixIcon: Icon(Icons.settings_ethernet),
              ),
            ),
            const SizedBox(height: 12),
            Row(
              children: [
                Expanded(
                  child: TextField(
                    controller: _user,
                    enabled: !connected && !app.busy,
                    decoration: const InputDecoration(
                      labelText: 'Usuário (opcional)',
                      prefixIcon: Icon(Icons.person_outline),
                    ),
                  ),
                ),
                const SizedBox(width: 10),
                Expanded(
                  child: TextField(
                    controller: _pass,
                    enabled: !connected && !app.busy,
                    obscureText: !_showPass,
                    decoration: InputDecoration(
                      labelText: 'Senha (opcional)',
                      prefixIcon: const Icon(Icons.lock_outline),
                      suffixIcon: IconButton(
                        icon: Icon(_showPass
                            ? Icons.visibility_off
                            : Icons.visibility),
                        onPressed: () =>
                            setState(() => _showPass = !_showPass),
                      ),
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 8),
            const Text(
              'Deixe usuário e senha em branco para acesso anônimo (padrão do FTPD). A senha fica apenas na memória.',
              style: TextStyle(color: Colors.white54, fontSize: 12),
            ),
            const SizedBox(height: 18),
            Wrap(
              spacing: 10,
              runSpacing: 10,
              alignment: WrapAlignment.center,
              children: [
                OutlinedButton.icon(
                  onPressed: (connected || app.busy || _testing)
                      ? null
                      : () => _test(app),
                  icon: _testing
                      ? const SizedBox(
                          width: 16,
                          height: 16,
                          child: CircularProgressIndicator(strokeWidth: 2),
                        )
                      : const Icon(Icons.network_check),
                  label: const Text('Testar conexão'),
                ),
                if (!connected)
                  FilledButton.icon(
                    onPressed: (app.busy || _testing) ? null : () => _connect(app),
                    icon: app.busy
                        ? const SizedBox(
                            width: 16,
                            height: 16,
                            child: CircularProgressIndicator(strokeWidth: 2),
                          )
                        : const Icon(Icons.link),
                    label: Text(app.busy ? 'Conectando…' : 'Conectar'),
                  )
                else
                  FilledButton.icon(
                    style: FilledButton.styleFrom(
                      backgroundColor: Colors.redAccent,
                    ),
                    onPressed: app.busy ? null : app.disconnect,
                    icon: const Icon(Icons.link_off),
                    label: const Text('Desconectar'),
                  ),
              ],
            ),
            if (_testResult != null) ...[
              const SizedBox(height: 14),
              Card(
                child: ListTile(
                  leading: Icon(
                    _testOk ? Icons.check_circle : Icons.error,
                    color: _testOk ? Colors.greenAccent : Colors.redAccent,
                  ),
                  title: Text(_testResult!),
                ),
              ),
            ],
            const SizedBox(height: 22),
            const Text(
              'O FTP tradicional não é criptografado. Use somente em Wi-Fi local confiável e nunca exponha a porta à internet.',
              style: TextStyle(color: Colors.white54, fontSize: 12),
              textAlign: TextAlign.center,
            ),
          ],
        ),
      ),
    );
  }
}
