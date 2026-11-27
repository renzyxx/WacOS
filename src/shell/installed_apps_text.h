#pragma once

#include "../comum/idioma.h"

namespace installed_apps_text {
enum Message {
    Title, Applications, Description, Name, Kind, LastLaunch, Size, OpenApplications,
    ShowInExplorer, Delete, Done, Close, UninstallApp, ConfirmUninstall, WindowsFallback,
    UninstallError, Cancel, OpenSettings, Ok, Minimize, Zoom, Restore, Count
};

inline const wchar_t* Get(Message message) {
    // Columns follow idioma::Lang, including languages displayed later in the settings menu.
    static constexpr const wchar_t* strings[][idioma::COUNT] = {
        {L"Aplicativos instalados", L"Installed Applications", L"Aplicaciones instaladas", L"Applications installées", L"Installierte Apps", L"Applicazioni installate", L"インストール済みアプリ", L"已安装的应用", L"설치된 앱", L"Установленные приложения", L"Zainstalowane aplikacje", L"Yüklü uygulamalar"},
        {L"Aplicativos", L"Applications", L"Aplicaciones", L"Applications", L"Apps", L"Applicazioni", L"アプリケーション", L"应用程序", L"응용 프로그램", L"Приложения", L"Aplikacje", L"Uygulamalar"},
        {L"Remova aplicativos para liberar espaço.", L"Remove applications to free up storage space.", L"Elimina aplicaciones para liberar espacio.", L"Supprimez des applications pour libérer de l’espace.", L"Entferne Apps, um Speicherplatz freizugeben.", L"Rimuovi le applicazioni per liberare spazio.", L"アプリを削除してストレージの空き容量を増やせます。", L"移除应用以释放存储空间。", L"앱을 제거하여 저장 공간을 확보하세요.", L"Удалите приложения, чтобы освободить место.", L"Usuń aplikacje, aby zwolnić miejsce.", L"Depolama alanı açmak için uygulamaları kaldırın."},
        {L"Nome", L"Name", L"Nombre", L"Nom", L"Name", L"Nome", L"名前", L"名称", L"이름", L"Название", L"Nazwa", L"Ad"},
        {L"Tipo", L"Kind", L"Tipo", L"Type", L"Typ", L"Tipo", L"種類", L"类型", L"종류", L"Тип", L"Typ", L"Tür"},
        {L"Última abertura", L"Last Opened", L"Última apertura", L"Dernière ouverture", L"Zuletzt geöffnet", L"Ultima apertura", L"最後に開いた日時", L"上次打开", L"마지막 실행", L"Последний запуск", L"Ostatnie uruchomienie", L"Son açılış"},
        {L"Tamanho", L"Size", L"Tamaño", L"Taille", L"Größe", L"Dimensioni", L"サイズ", L"大小", L"크기", L"Размер", L"Rozmiar", L"Boyut"},
        {L"Abrir Aplicativos", L"Open Applications", L"Abrir aplicaciones", L"Ouvrir Applications", L"Apps öffnen", L"Apri Applicazioni", L"アプリケーションを開く", L"打开应用程序", L"응용 프로그램 열기", L"Открыть приложения", L"Otwórz aplikacje", L"Uygulamaları aç"},
        {L"Mostrar no Explorador", L"Show in Explorer", L"Mostrar en el Explorador", L"Afficher dans l’Explorateur", L"Im Explorer anzeigen", L"Mostra in Esplora file", L"エクスプローラーで表示", L"在资源管理器中显示", L"파일 탐색기에서 보기", L"Показать в Проводнике", L"Pokaż w Eksploratorze", L"Dosya Gezgini’nde göster"},
        {L"Remover…", L"Delete…", L"Eliminar…", L"Supprimer…", L"Entfernen…", L"Rimuovi…", L"削除…", L"移除…", L"제거…", L"Удалить…", L"Usuń…", L"Kaldır…"},
        {L"Concluído", L"Done", L"Listo", L"Terminé", L"Fertig", L"Fine", L"完了", L"完成", L"완료", L"Готово", L"Gotowe", L"Bitti"},
        {L"Fechar", L"Close", L"Cerrar", L"Fermer", L"Schließen", L"Chiudi", L"閉じる", L"关闭", L"닫기", L"Закрыть", L"Zamknij", L"Kapat"},
        {L"Desinstalar aplicativo…", L"Uninstall App…", L"Desinstalar aplicación…", L"Désinstaller l’application…", L"App deinstallieren…", L"Disinstalla applicazione…", L"アプリをアンインストール…", L"卸载应用…", L"앱 제거…", L"Удалить приложение…", L"Odinstaluj aplikację…", L"Uygulamayı kaldır…"},
        {L"Desinstalar «%ls»?", L"Uninstall “%ls”?", L"¿Desinstalar «%ls»?", L"Désinstaller « %ls » ?", L"„%ls“ deinstallieren?", L"Disinstallare «%ls»?", L"「%ls」をアンインストールしますか？", L"要卸载“%ls”吗？", L"‘%ls’ 앱을 제거하시겠습니까?", L"Удалить «%ls»?", L"Odinstalować „%ls”?", L"“%ls” kaldırılsın mı?"},
        {L"Abra os Aplicativos instalados do Windows para desinstalar este aplicativo.", L"Open Windows Installed Apps to uninstall this application.", L"Abre Aplicaciones instaladas de Windows para desinstalar esta aplicación.", L"Ouvrez les applications installées de Windows pour désinstaller cette application.", L"Öffne die installierten Apps in Windows, um diese App zu deinstallieren.", L"Apri App installate di Windows per disinstallare questa applicazione.", L"このアプリを削除するには、Windows の「インストールされているアプリ」を開いてください。", L"请打开 Windows 的“已安装的应用”来卸载此应用。", L"이 앱을 제거하려면 Windows의 설치된 앱을 여세요.", L"Чтобы удалить это приложение, откройте список установленных приложений Windows.", L"Otwórz Zainstalowane aplikacje systemu Windows, aby odinstalować tę aplikację.", L"Bu uygulamayı kaldırmak için Windows Yüklü uygulamalar sayfasını açın."},
        {L"Não foi possível abrir o desinstalador.", L"Could not open the uninstaller.", L"No se pudo abrir el desinstalador.", L"Impossible d’ouvrir le programme de désinstallation.", L"Das Deinstallationsprogramm konnte nicht geöffnet werden.", L"Impossibile aprire il programma di disinstallazione.", L"アンインストーラーを開けませんでした。", L"无法打开卸载程序。", L"제거 프로그램을 열 수 없습니다.", L"Не удалось запустить деинсталлятор.", L"Nie udało się otworzyć deinstalatora.", L"Kaldırma programı açılamadı."},
        {L"Cancelar", L"Cancel", L"Cancelar", L"Annuler", L"Abbrechen", L"Annulla", L"キャンセル", L"取消", L"취소", L"Отмена", L"Anuluj", L"İptal"},
        {L"Abrir Configurações", L"Open Settings", L"Abrir Configuración", L"Ouvrir les paramètres", L"Einstellungen öffnen", L"Apri Impostazioni", L"設定を開く", L"打开设置", L"설정 열기", L"Открыть настройки", L"Otwórz ustawienia", L"Ayarları aç"},
        {L"OK", L"OK", L"Aceptar", L"OK", L"OK", L"OK", L"OK", L"确定", L"확인", L"ОК", L"OK", L"Tamam"},
        {L"Minimizar", L"Minimize", L"Minimizar", L"Réduire", L"Minimieren", L"Riduci a icona", L"最小化", L"最小化", L"최소화", L"Свернуть", L"Minimalizuj", L"Simge durumuna küçült"},
        {L"Maximizar", L"Maximize", L"Maximizar", L"Agrandir", L"Maximieren", L"Ingrandisci", L"最大化", L"最大化", L"최대화", L"Развернуть", L"Maksymalizuj", L"Ekranı kapla"},
        {L"Restaurar tamanho", L"Restore Size", L"Restaurar tamaño", L"Rétablir la taille", L"Größe wiederherstellen", L"Ripristina dimensioni", L"サイズを元に戻す", L"还原大小", L"크기 복원", L"Восстановить размер", L"Przywróć rozmiar", L"Boyutu geri yükle"},
    };
    static_assert(ARRAYSIZE(strings) == Count);
    return strings[message][idioma::Current()];
}
}
