/// アプリケーション
/// シングルトンクラス
#include <memory>

struct Size {
	Size() {};
	Size(int inw, int inh) :w(inw), h(inh) {};
	int w;
	int h;
};

class Dx12Initer;

class Application
{
	
private:
	std::shared_ptr<Dx12Initer> _dx;
	void InitWindow();
	Application();								/// 生成禁止
	Application(const Application&) {};			/// コピー禁止
	void operator=(const Application&) {};		/// 代入禁止
public:
	Size size;
	static Application& Instance() {
		static Application instance;
		return instance;
	}
	~Application();
	/// 初期化処理
	void Initialize();
	/// メイン処理
	void Run();
	/// 後処理
	void Terminate();

	Size GetSize() const;
};

