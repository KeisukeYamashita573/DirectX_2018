/// �A�v���P�[�V����
/// �V���O���g���N���X
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
	Application();								/// �����֎~
	Application(const Application&) {};			/// �R�s�[�֎~
	void operator=(const Application&) {};		/// ����֎~
public:
	Size size;
	static Application& Instance() {
		static Application instance;
		return instance;
	}
	~Application();
	/// ����������
	void Initialize();
	/// ���C������
	void Run();
	/// �㏈��
	void Terminate();

	Size GetSize() const;
};

