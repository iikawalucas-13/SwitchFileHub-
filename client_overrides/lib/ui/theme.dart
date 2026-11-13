import 'package:flutter/material.dart';

import '../state/app_controller.dart';

const kBg = Color(0xFF090B12);
const kSurface = Color(0xFF111622);
const kSurface2 = Color(0xFF171F2E);
const kBlue = Color(0xFF4B8DFF);
const kPurple = Color(0xFF906BFF);
const kFolder = Color(0xFFFFC857);

ThemeData buildTheme() {
  final scheme = ColorScheme.fromSeed(
    seedColor: kBlue,
    brightness: Brightness.dark,
  ).copyWith(
    primary: kBlue,
    secondary: kPurple,
    surface: kSurface,
    onSurface: const Color(0xFFE8ECF5),
  );
  final outline = const Color(0xFF30394B);
  return ThemeData(
    useMaterial3: true,
    brightness: Brightness.dark,
    colorScheme: scheme,
    scaffoldBackgroundColor: kBg,
    cardTheme: CardThemeData(
      color: kSurface2,
      elevation: 0,
      margin: const EdgeInsets.symmetric(vertical: 5),
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(18),
        side: BorderSide(color: outline.withValues(alpha: .8)),
      ),
    ),
    appBarTheme: const AppBarTheme(
      backgroundColor: kBg,
      surfaceTintColor: Colors.transparent,
      elevation: 0,
      centerTitle: false,
    ),
    navigationRailTheme: const NavigationRailThemeData(
      backgroundColor: kSurface,
      indicatorColor: Color(0x338B5CF6),
    ),
    navigationDrawerTheme: const NavigationDrawerThemeData(
      backgroundColor: kSurface,
      indicatorColor: Color(0x338B5CF6),
    ),
    inputDecorationTheme: InputDecorationTheme(
      filled: true,
      fillColor: const Color(0xFF0E131E),
      border: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: outline),
      ),
      enabledBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: BorderSide(color: outline),
      ),
      focusedBorder: OutlineInputBorder(
        borderRadius: BorderRadius.circular(14),
        borderSide: const BorderSide(color: kBlue, width: 1.6),
      ),
      contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 16),
    ),
    filledButtonTheme: FilledButtonThemeData(
      style: FilledButton.styleFrom(
        minimumSize: const Size(0, 48),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
      ),
    ),
    outlinedButtonTheme: OutlinedButtonThemeData(
      style: OutlinedButton.styleFrom(
        minimumSize: const Size(0, 48),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
      ),
    ),
  );
}

class AppScope extends InheritedNotifier<AppController> {
  const AppScope({
    super.key,
    required AppController controller,
    required super.child,
  }) : super(notifier: controller);

  static AppController of(BuildContext context) {
    final scope = context.dependOnInheritedWidgetOfExactType<AppScope>();
    assert(scope != null, 'AppScope ausente');
    return scope!.notifier!;
  }
}

void showMsg(BuildContext context, String text) {
  ScaffoldMessenger.of(context)
    ..hideCurrentSnackBar()
    ..showSnackBar(SnackBar(content: Text(text)));
}

Future<bool> confirmDialog(
  BuildContext context, {
  required String title,
  required String message,
  String confirmLabel = 'Continuar',
  bool danger = false,
}) async {
  final r = await showDialog<bool>(
    context: context,
    builder: (ctx) => AlertDialog(
      title: Text(title),
      content: SingleChildScrollView(child: Text(message)),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(ctx, false),
          child: const Text('Cancelar'),
        ),
        FilledButton(
          style: danger
              ? FilledButton.styleFrom(backgroundColor: Colors.redAccent)
              : null,
          onPressed: () => Navigator.pop(ctx, true),
          child: Text(confirmLabel),
        ),
      ],
    ),
  );
  return r ?? false;
}

Future<String?> promptDialog(
  BuildContext context, {
  required String title,
  required String label,
  String initial = '',
  String confirmLabel = 'OK',
}) async {
  final c = TextEditingController(text: initial);
  final r = await showDialog<String>(
    context: context,
    builder: (ctx) => AlertDialog(
      title: Text(title),
      content: TextField(
        controller: c,
        autofocus: true,
        decoration: InputDecoration(labelText: label),
        onSubmitted: (v) => Navigator.pop(ctx, v.trim()),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(ctx),
          child: const Text('Cancelar'),
        ),
        FilledButton(
          onPressed: () => Navigator.pop(ctx, c.text.trim()),
          child: Text(confirmLabel),
        ),
      ],
    ),
  );
  c.dispose();
  return (r == null || r.isEmpty) ? null : r;
}

Future<OverwriteChoice> conflictDialog(
  BuildContext context,
  List<String> conflicts,
) async {
  final shown = conflicts.take(8).join('\n');
  final more = conflicts.length > 8 ? '\n… e mais ${conflicts.length - 8}' : '';
  final r = await showDialog<OverwriteChoice>(
    context: context,
    builder: (ctx) => AlertDialog(
      title: const Text('Itens já existem'),
      content: Text('Estes itens já existem no destino:\n\n$shown$more\n\nO que deseja fazer?'),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(ctx, OverwriteChoice.cancel),
          child: const Text('Cancelar'),
        ),
        OutlinedButton(
          onPressed: () => Navigator.pop(ctx, OverwriteChoice.skipExisting),
          child: const Text('Pular existentes'),
        ),
        FilledButton(
          onPressed: () => Navigator.pop(ctx, OverwriteChoice.overwrite),
          child: const Text('Substituir'),
        ),
      ],
    ),
  );
  return r ?? OverwriteChoice.cancel;
}
